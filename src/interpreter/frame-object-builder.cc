// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/frame-object-builder.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>

#include "include/saauso-internal.h"
#include "src/interpreter/frame-object.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {
namespace {

struct FrameBuildContext {
  // 目标函数对应的 code object。
  Handle<PyCodeObject> code_object;
  // 目标函数
  Handle<PyFunction> func;
  // locals/globals：当前栈帧可见的变量表（locals 用于泄露符号名，globals
  // 通常来自 func）。
  Handle<PyDict> locals;
  Handle<PyDict> globals;
  // localsplus：依次放置以下内容
  // - 局部变量（包括形参槽位、以及可能注入的 *args/**kwargs）。
  // - Cell变量
  // - Free变量
  Handle<FixedArray> localsplus;
  // operand stack：解释器执行期间的操作数栈。
  Handle<FixedArray> stack;
  // 仅用于打印错误信息（TypeError: {func_name} ...）。
  Handle<PyString> func_name;
  // 形参符号表（包含函数形参），用于 kw 形参匹配：index_in_var_args =
  // var_names->IndexOf(key)。
  Handle<PyTuple> var_names;
  // 不算 *args 和 **kwargs 的形参个数（但可能包含 self）。
  int64_t real_formal_pos_arg_cnt{0};
  // 不算 self、*args 和 **kwargs 的“用户可见”形参个数。
  int64_t formal_pos_arg_cnt{0};
  // 若 host 非空，则注入为首个参数（self），此处记录 self 的个数（0/1）。
  int self_arg_cnt{0};
  // localsplus 的“写入游标”：
  // 指向下一个可放置元素的位置（用于顺序写入位置实参、以及逻辑上统计已填充的形参槽位）。
  int localsplus_idx{0};
};

Handle<PyDict> GetDefaultBoundLocals(Handle<PyFunction> func) {
  if ((func->flags() & PyCodeObject::Flag::kOptimized) != 0) [[unlikely]] {
    return func->func_globals();
  }
  return Handle<PyDict>::null();
}

FrameBuildContext PrepareForFunction(Handle<PyFunction> func,
                                     Handle<PyObject> host,
                                     Handle<PyDict> bound_locals) {
  FrameBuildContext ctx;
  ctx.code_object = func->func_code();
  ctx.func = func;
  ctx.locals = bound_locals;
  ctx.globals = func->func_globals();
  if (ctx.code_object->nlocalsplus() > 0) {
    ctx.localsplus = FixedArray::NewInstance(ctx.code_object->nlocalsplus());
  }
  ctx.stack = FixedArray::NewInstance(ctx.code_object->stack_size());

  ctx.func_name = ctx.code_object->co_name();
  ctx.var_names = ctx.code_object->var_names();

  ctx.real_formal_pos_arg_cnt = ctx.code_object->arg_count();

  // 将 host 填充为首个函数参数（self）。
  if (!host.is_null()) {
    ctx.localsplus->Set(ctx.localsplus_idx++, host);
    ctx.self_arg_cnt = 1;
  }
  // 不算 self、*args 和 **kwargs 的形参个数。
  ctx.formal_pos_arg_cnt = ctx.real_formal_pos_arg_cnt - ctx.self_arg_cnt;

  return ctx;
}

void FillDefaultArgs(FrameBuildContext& ctx, Handle<PyTuple> default_args) {
  // 使用默认值填充 localsplus 中的空洞（从尾部形参开始向前回填）。
  if (default_args.is_null()) {
    return;
  }

  int64_t default_arg_cnt = default_args->length();
  int64_t arg_list_index = ctx.real_formal_pos_arg_cnt - 1;
  while (default_arg_cnt > 0) {
    if (ctx.localsplus->Get(arg_list_index).is_null()) {
      ctx.localsplus->Set(arg_list_index,
                          default_args->Get(default_arg_cnt - 1));
      // 同理，让 localsplus_idx 游标在逻辑上向右移动一下。
      ++ctx.localsplus_idx;
    }
    --default_arg_cnt;
    --arg_list_index;
  }
}

void CheckMissingArgs(const FrameBuildContext& ctx) {
  // 如果还有位置形参没有有效值，那么抛出错误。
  if (ctx.localsplus_idx < ctx.real_formal_pos_arg_cnt) {
    std::fprintf(stderr,
                 "TypeError: %s() missing %" PRId64
                 " required positional argument\n",
                 ctx.func_name->buffer(),
                 ctx.real_formal_pos_arg_cnt - ctx.localsplus_idx);
    std::exit(1);
  }
}

int64_t ComputeExtraPosArgCountFromPosArgs(int64_t actual_pos_cnt,
                                           int64_t formal_pos_arg_cnt) {
  // 注意：当 actual_pos_cnt < formal_pos_arg_cnt 时，extra 应为 0；
  return std::max<int64_t>(0, actual_pos_cnt - formal_pos_arg_cnt);
}

Handle<PyTuple> PackExtraPosArgsFromPosArgs(FrameBuildContext& ctx,
                                            Handle<PyTuple> actual_pos_args) {
  int64_t actual_pos_cnt =
      actual_pos_args.is_null() ? 0 : actual_pos_args->length();
  int64_t extra_pos_cnt = ComputeExtraPosArgCountFromPosArgs(
      actual_pos_cnt, ctx.formal_pos_arg_cnt);

  Handle<PyTuple> extend_pos_args;
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarArgs) {
    extend_pos_args = PyTuple::NewInstance(extra_pos_cnt);
  }

  if (!actual_pos_args.is_null() && extra_pos_cnt > 0) {
    // 打包拓展参数：如果函数不接受扩展参数，直接报错。
    if (extend_pos_args.is_null()) {
      std::fprintf(stderr,
                   "TypeError: %s() takes %" PRId64
                   " positional arguments but %" PRId64 " were given",
                   ctx.func_name->buffer(), ctx.real_formal_pos_arg_cnt,
                   actual_pos_cnt + ctx.self_arg_cnt);
      std::exit(1);
    }

    for (int64_t i = 0; i < extra_pos_cnt; ++i) {
      extend_pos_args->SetInternal(
          i, actual_pos_args->Get(ctx.formal_pos_arg_cnt + i));
    }
  }

  return extend_pos_args;
}

Handle<PyTuple> PackExtraPosArgsFromActualArgs(FrameBuildContext& ctx,
                                               Handle<PyTuple> actual_args,
                                               int64_t actual_kw_arg_cnt) {
  int64_t actual_arg_cnt = actual_args.is_null() ? 0 : actual_args->length();
  int64_t actual_pos_cnt = actual_arg_cnt - actual_kw_arg_cnt;
  int64_t extra_pos_cnt = ComputeExtraPosArgCountFromPosArgs(
      actual_pos_cnt, ctx.formal_pos_arg_cnt);

  Handle<PyTuple> extend_pos_args;
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarArgs) {
    extend_pos_args = PyTuple::NewInstance(extra_pos_cnt);
  }

  if (extra_pos_cnt > 0) {
    // 打包拓展参数：如果函数不接受扩展参数，直接报错。
    if (extend_pos_args.is_null()) {
      std::fprintf(stderr,
                   "TypeError: %s() takes %" PRId64
                   " positional arguments but %" PRId64 " were given",
                   ctx.func_name->buffer(), ctx.real_formal_pos_arg_cnt,
                   actual_arg_cnt + ctx.self_arg_cnt);
      std::exit(1);
    }

    for (int64_t i = 0; i < extra_pos_cnt; ++i) {
      extend_pos_args->SetInternal(
          i, actual_args->Get(ctx.formal_pos_arg_cnt + i));
    }
  }

  return extend_pos_args;
}

void InjectVarArgsAndKwArgs(FrameBuildContext& ctx,
                            Handle<PyTuple> var_args,
                            Handle<PyDict> kw_args) {
  // 向 localsplus 中注入 extend_pos_args（*args）。
  if (!var_args.is_null()) {
    ctx.localsplus->Set(ctx.localsplus_idx++, var_args);
  }
  // 向 localsplus 中注入 kw_args（**kwargs）。
  if (!kw_args.is_null()) {
    ctx.localsplus->Set(ctx.localsplus_idx++, kw_args);
  }
}

}  // namespace

FrameObject* FrameObjectBuilder::BuildSlowPath(Handle<PyFunction> func,
                                               Handle<PyObject> host,
                                               Handle<PyTuple> actual_pos_args,
                                               Handle<PyDict> actual_kw_args) {
  return BuildSlowPath(func, host, actual_pos_args, actual_kw_args,
                       GetDefaultBoundLocals(func));
}

FrameObject* FrameObjectBuilder::BuildSlowPath(Handle<PyFunction> func,
                                               Handle<PyObject> host,
                                               Handle<PyTuple> actual_pos_args,
                                               Handle<PyDict> actual_kw_args,
                                               Handle<PyDict> bound_locals) {
  // 在build入口设置一个scope。
  // 构建中途需避免再建handle scope，避免FrameBuildContext中的handle失效
  HandleScope scope;

  // 创建一般的 python 栈帧（慢速路径）。
  FrameBuildContext ctx = PrepareForFunction(func, host, bound_locals);

  // 将与形参对应的函数实参加载到栈帧的 localsplus 上去（位置实参优先）。
  if (!actual_pos_args.is_null()) {
    int64_t valid_pos_args_cnt =
        std::min<int64_t>(actual_pos_args->length(), ctx.formal_pos_arg_cnt);
    for (int64_t i = 0; i < valid_pos_args_cnt; ++i) {
      ctx.localsplus->Set(ctx.localsplus_idx++, actual_pos_args->Get(i));
    }
  }

  Handle<PyDict> kw_args;
  // 如果函数支持接收 **kwargs，在这里初始化它。
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarKeywords) {
    kw_args = PyDict::NewInstance();
  }

  // 尝试使用 actual_kw_args 填充没有通过位置实参赋值的形参。
  if (!actual_kw_args.is_null()) {
    // 遍历 actual_kw_args，看看是否是对函数的形参进行赋值：
    // 1. 如果是，那么注入到形参；
    // 2. 如果不是：若函数支持 **kwargs，则整理到 kw_args 当中去；否则抛出错误。
    auto iter = PyDictItemIterator::NewInstance(actual_kw_args);
    auto item = Handle<PyTuple>::cast(PyObject::Next(iter));
    while (!item.is_null()) {
      auto key = Handle<PyString>::cast(item->Get(0));
      auto value = item->Get(1);
      int64_t index_in_var_args =
          ctx.var_names->IndexOf(key, 0, ctx.real_formal_pos_arg_cnt);

      // 如果当前 key 对应于形参列表中的某个形参。
      if (index_in_var_args != PyTuple::kNotFound) {
        // 不允许重复对形参赋值。
        if (!ctx.localsplus->Get(index_in_var_args).is_null()) {
          std::fprintf(
              stderr, "TypeError: %s() got multiple values for argument '%s'\n",
              ctx.func_name->buffer(), key->buffer());
          std::exit(1);
        }

        // 给形参赋值。
        ctx.localsplus->Set(index_in_var_args, value);
        // 现在有新的形参被填充进 localsplus 了，
        // 因此让 localsplus_idx 游标在逻辑上向右移动一下。
        ++ctx.localsplus_idx;
      } else if (!kw_args.is_null()) {
        // key 没有命中特定的形参，如果当前函数支持键值对参数包，
        // 将键值对参数注入进 **kwargs 当中。
        PyDict::Put(kw_args, key, value);
      } else {
        // 键值对参数既没有命中形参，函数也不支持接收键值对参数包，那么抛出错误。
        std::fprintf(
            stderr, "TypeError: %s() got an unexpected keyword argument '%s'\n",
            ctx.func_name->buffer(), key->buffer());
        std::exit(1);
      }

      // 刷新迭代器。
      item = Handle<PyTuple>::cast(PyObject::Next(iter));
    }
  }

  FillDefaultArgs(ctx, func->default_args());
  CheckMissingArgs(ctx);

  // 打包拓展参数（*args），并注入到 fast_locals。
  Handle<PyTuple> extend_pos_args =
      PackExtraPosArgsFromPosArgs(ctx, actual_pos_args);
  // 注入 *args/**kwargs。
  InjectVarArgsAndKwArgs(ctx, extend_pos_args, kw_args);

  return FrameObject::Create(
      *ctx.code_object->consts(), *ctx.code_object->names(), *ctx.locals,
      *ctx.globals, *ctx.localsplus, *ctx.stack, *ctx.code_object, *ctx.func);
}

FrameObject* FrameObjectBuilder::BuildFastPath(Handle<PyFunction> func,
                                               Handle<PyObject> host,
                                               Handle<PyTuple> actual_args,
                                               Handle<PyTuple> kwarg_keys) {
  return BuildFastPath(func, host, actual_args, kwarg_keys,
                       GetDefaultBoundLocals(func));
}

FrameObject* FrameObjectBuilder::BuildFastPath(Handle<PyFunction> func,
                                               Handle<PyObject> host,
                                               Handle<PyTuple> actual_args,
                                               Handle<PyTuple> kwarg_keys,
                                               Handle<PyDict> bound_locals) {
  // 在build入口设置一个scope。
  // 构建中途需避免再建handle scope，避免FrameBuildContext中的handle失效
  HandleScope scope;

  // 创建一般的 python 栈帧（快速路径）。
  FrameBuildContext ctx = PrepareForFunction(func, host, bound_locals);

  // 用户传入的全体实参个数。
  int64_t actual_arg_cnt = actual_args.is_null() ? 0 : actual_args->length();
  // 用户通过键值对形式传入的实参个数。
  int64_t actual_kw_arg_cnt = kwarg_keys.is_null() ? 0 : kwarg_keys->length();

  // 将与形参对应的函数实参加载到栈帧的 localsplus 上去（位置部分）。
  int64_t valid_pos_args_cnt = std::min<int64_t>(
      actual_arg_cnt - actual_kw_arg_cnt, ctx.formal_pos_arg_cnt);
  for (int64_t i = 0; i < valid_pos_args_cnt; ++i) {
    ctx.localsplus->Set(ctx.localsplus_idx++, actual_args->Get(i));
  }

  Handle<PyDict> kw_args;
  // 如果函数支持接收 **kwargs，在这里初始化它。
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarKeywords) {
    kw_args = PyDict::NewInstance();
  }

  // 遍历 kwarg_keys：
  // - 约定：kw 的 value 位于 actual_args 尾部，key/value 从尾部一一对应取出。
  for (int64_t i = 0; i < actual_kw_arg_cnt; ++i) {
    auto key =
        Handle<PyString>::cast(kwarg_keys->Get(actual_kw_arg_cnt - i - 1));
    auto value = actual_args->Get(actual_arg_cnt - i - 1);

    // 1. 检查当前的键值对是否能够赋值给特定的形参。
    int64_t index_in_var_args =
        ctx.var_names->IndexOf(key, 0, ctx.real_formal_pos_arg_cnt);
    if (index_in_var_args != PyTuple::kNotFound) {
      // 不允许重复对形参赋值。
      if (!ctx.localsplus->Get(index_in_var_args).is_null()) {
        std::fprintf(stderr,
                     "TypeError: %s() got multiple values for argument '%s'\n",
                     ctx.func_name->buffer(), key->buffer());
        std::exit(1);
      }

      // 检查通过，给形参赋值。
      ctx.localsplus->Set(index_in_var_args, value);
      // 现在有新的形参被填充进 localsplus 了，
      // 因此让 localsplus_idx 游标在逻辑上向右移动一下。
      ++ctx.localsplus_idx;
      continue;
    }

    // 2. 没有命中特定的形参，如果当前函数支持键值对参数包，
    //    将键值对参数注入进 **kwargs 当中。
    if (!kw_args.is_null()) {
      PyDict::Put(kw_args, key, value);
      continue;
    }

    // 3. 键值对参数既没有命中形参，函数也不支持接收键值对参数包，那么抛出错误。
    std::fprintf(stderr,
                 "TypeError: %s() got an unexpected keyword argument '%s'\n",
                 ctx.func_name->buffer(), key->buffer());
    std::exit(1);
  }

  FillDefaultArgs(ctx, func->default_args());
  CheckMissingArgs(ctx);

  // 打包拓展参数（*args），并注入到 fast_locals。
  Handle<PyTuple> extend_pos_args =
      PackExtraPosArgsFromActualArgs(ctx, actual_args, actual_kw_arg_cnt);
  // 注入 *args/**kwargs。
  InjectVarArgsAndKwArgs(ctx, extend_pos_args, kw_args);

  return FrameObject::Create(
      *ctx.code_object->consts(), *ctx.code_object->names(), *ctx.locals,
      *ctx.globals, *ctx.localsplus, *ctx.stack, *ctx.code_object, *ctx.func);
}

}  // namespace saauso::internal
