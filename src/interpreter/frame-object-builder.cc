// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/frame-object-builder.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/interpreter/frame-object.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
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

void LoadPosActualArgsToLocalsplus(FrameBuildContext& ctx,
                                   Handle<PyTuple> actual_args,
                                   int64_t actual_pos_args_cnt) {
  int64_t valid_pos_args_cnt =
      std::min(actual_pos_args_cnt, ctx.formal_pos_arg_cnt);
  for (int64_t i = 0; i < valid_pos_args_cnt; ++i) {
    ctx.localsplus->Set(ctx.localsplus_idx++, actual_args->Get(i));
  }
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

// 检查是否有位置形参没有有效值。
// 若缺失形参，抛出 TypeError 并返回 false；否则返回 true。
bool CheckMissingArgs(const FrameBuildContext& ctx) {
  if (ctx.localsplus_idx < ctx.real_formal_pos_arg_cnt) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "%s() missing %" PRId64 " required positional argument",
                        ctx.func_name->buffer(),
                        ctx.real_formal_pos_arg_cnt - ctx.localsplus_idx);
    return false;
  }
  return true;
}

int64_t ComputeExtraPosArgCountFromPosArgs(int64_t actual_pos_cnt,
                                           int64_t formal_pos_arg_cnt) {
  // 注意：当 actual_pos_cnt < formal_pos_arg_cnt 时，extra 应为 0；
  return std::max<int64_t>(0, actual_pos_cnt - formal_pos_arg_cnt);
}

bool PackExtraPosArgsFromPosArgs(const FrameBuildContext& ctx,
                                 Handle<PyTuple> actual_pos_args,
                                 Handle<PyTuple>& extend_pos_args) {
  int64_t actual_pos_cnt =
      actual_pos_args.is_null() ? 0 : actual_pos_args->length();
  int64_t extra_pos_cnt = ComputeExtraPosArgCountFromPosArgs(
      actual_pos_cnt, ctx.formal_pos_arg_cnt);

  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarArgs) {
    extend_pos_args = PyTuple::NewInstance(extra_pos_cnt);
  }

  if (!actual_pos_args.is_null() && extra_pos_cnt > 0) {
    // 打包拓展参数：如果函数不接受扩展参数，直接报错。
    if (extend_pos_args.is_null()) {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "%s() takes %" PRId64
                          " positional arguments but %" PRId64 " were given",
                          ctx.func_name->buffer(), ctx.real_formal_pos_arg_cnt,
                          actual_pos_cnt + ctx.self_arg_cnt);
      return false;
    }

    for (int64_t i = 0; i < extra_pos_cnt; ++i) {
      extend_pos_args->SetInternal(
          i, actual_pos_args->Get(ctx.formal_pos_arg_cnt + i));
    }
  }

  return true;
}

bool PackExtraPosArgsFromActualArgs(const FrameBuildContext& ctx,
                                    Handle<PyTuple> actual_args,
                                    int64_t actual_kw_arg_cnt,
                                    Handle<PyTuple>& extend_pos_args) {
  int64_t actual_arg_cnt = actual_args.is_null() ? 0 : actual_args->length();
  int64_t actual_pos_cnt = actual_arg_cnt - actual_kw_arg_cnt;
  int64_t extra_pos_cnt = ComputeExtraPosArgCountFromPosArgs(
      actual_pos_cnt, ctx.formal_pos_arg_cnt);

  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarArgs) {
    extend_pos_args = PyTuple::NewInstance(extra_pos_cnt);
  }

  if (extra_pos_cnt > 0) {
    // 打包拓展参数：如果函数不接受扩展参数，直接报错。
    if (extend_pos_args.is_null()) {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "%s() takes %" PRId64
                          " positional arguments but %" PRId64 " were given",
                          ctx.func_name->buffer(), ctx.real_formal_pos_arg_cnt,
                          actual_arg_cnt + ctx.self_arg_cnt);
      return false;
    }

    for (int64_t i = 0; i < extra_pos_cnt; ++i) {
      extend_pos_args->SetInternal(
          i, actual_args->Get(ctx.formal_pos_arg_cnt + i));
    }
  }

  return true;
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

bool AssignKwArgsFromDict(FrameBuildContext& ctx,
                          Handle<PyDict> actual_kw_args,
                          Handle<PyDict>& out_kw_args) {
  // 如果函数支持接收 **kwargs，在这里初始化它。
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarKeywords) {
    out_kw_args = PyDict::NewInstance();
  }

  if (actual_kw_args.is_null()) {
    return true;
  }

  auto iter = PyDictItemIterator::NewInstance(actual_kw_args);
  auto* isolate = Isolate::Current();
  while (true) {
    Handle<PyObject> item_handle;
    if (!PyObject::Next(iter).ToHandle(&item_handle)) {
      if (Runtime_ConsumePendingStopIterationIfSet(isolate).ToChecked()) {
        break;
      }
      return false;
    }
    auto item = Handle<PyTuple>::cast(item_handle);
    auto key = Handle<PyString>::cast(item->Get(0));
    auto value = item->Get(1);
    int64_t index_in_var_args;
    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, index_in_var_args,
        ctx.var_names->IndexOf(key, 0, ctx.real_formal_pos_arg_cnt), false);

    if (index_in_var_args != PyTuple::kNotFound) {
      if (!ctx.localsplus->Get(index_in_var_args).is_null()) {
        Runtime_ThrowErrorf(ExceptionType::kTypeError,
                            "%s() got multiple values for argument '%s'",
                            ctx.func_name->buffer(), key->buffer());
        return false;
      }

      ctx.localsplus->Set(index_in_var_args, value);
      ++ctx.localsplus_idx;
    } else if (!out_kw_args.is_null()) {
      if (PyDict::PutMaybe(out_kw_args, key, value).IsNothing()) {
        return false;
      }
    } else {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "%s() got an unexpected keyword argument '%s'",
                          ctx.func_name->buffer(), key->buffer());
      return false;
    }
  }

  return true;
}

bool AssignKwArgsFromActualArgs(FrameBuildContext& ctx,
                                Handle<PyTuple> actual_args,
                                Handle<PyTuple> kwarg_keys,
                                Handle<PyDict>& out_kw_args) {
  // 如果函数支持接收 **kwargs，在这里初始化它。
  if (ctx.code_object->flags() & PyCodeObject::Flag::kVarKeywords) {
    out_kw_args = PyDict::NewInstance();
  }

  if (kwarg_keys.is_null()) {
    return true;
  }

  auto* isolate = Isolate::Current();
  int64_t actual_arg_cnt = actual_args.is_null() ? 0 : actual_args->length();
  int64_t actual_kw_arg_cnt = kwarg_keys->length();

  for (int64_t i = 0; i < actual_kw_arg_cnt; ++i) {
    auto key =
        Handle<PyString>::cast(kwarg_keys->Get(actual_kw_arg_cnt - i - 1));
    auto value = actual_args->Get(actual_arg_cnt - i - 1);

    int64_t index_in_var_args;
    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, index_in_var_args,
        ctx.var_names->IndexOf(key, 0, ctx.real_formal_pos_arg_cnt), false);
    if (index_in_var_args != PyTuple::kNotFound) {
      if (!ctx.localsplus->Get(index_in_var_args).is_null()) {
        Runtime_ThrowErrorf(ExceptionType::kTypeError,
                            "%s() got multiple values for argument '%s'",
                            ctx.func_name->buffer(), key->buffer());
        return false;
      }

      ctx.localsplus->Set(index_in_var_args, value);
      ++ctx.localsplus_idx;
      continue;
    }

    if (!out_kw_args.is_null()) {
      if (PyDict::PutMaybe(out_kw_args, key, value).IsNothing()) {
        return false;
      }
      continue;
    }

    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "%s() got an unexpected keyword argument '%s'",
                        ctx.func_name->buffer(), key->buffer());
    return false;
  }

  return true;
}

}  // namespace

Maybe<FrameObject*> FrameObjectBuilder::BuildSlowPath(
    Handle<PyFunction> func,
    Handle<PyObject> host,
    Handle<PyTuple> actual_pos_args,
    Handle<PyDict> actual_kw_args) {
  return BuildSlowPath(func, host, actual_pos_args, actual_kw_args,
                       GetDefaultBoundLocals(func));
}

Maybe<FrameObject*> FrameObjectBuilder::BuildSlowPath(
    Handle<PyFunction> func,
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
    LoadPosActualArgsToLocalsplus(ctx, actual_pos_args,
                                  actual_pos_args->length());
  }

  Handle<PyDict> kw_args;
  if (!AssignKwArgsFromDict(ctx, actual_kw_args, kw_args)) {
    return kNullMaybe;
  }

  FillDefaultArgs(ctx, func->default_args());
  if (!CheckMissingArgs(ctx)) {
    return kNullMaybe;
  }

  Handle<PyTuple> extend_pos_args;
  if (!PackExtraPosArgsFromPosArgs(ctx, actual_pos_args, extend_pos_args)) {
    return kNullMaybe;
  }

  // 注入 *args/**kwargs。
  InjectVarArgsAndKwArgs(ctx, extend_pos_args, kw_args);

  auto* frame_object = FrameObject::Create(
      *ctx.code_object->consts(), *ctx.code_object->names(), *ctx.locals,
      *ctx.globals, *ctx.localsplus, *ctx.stack, *ctx.code_object, *ctx.func);
  return Maybe<FrameObject*>(frame_object);
}

Maybe<FrameObject*> FrameObjectBuilder::BuildFastPath(
    Handle<PyFunction> func,
    Handle<PyObject> host,
    Handle<PyTuple> actual_args,
    Handle<PyTuple> kwarg_keys) {
  return BuildFastPath(func, host, actual_args, kwarg_keys,
                       GetDefaultBoundLocals(func));
}

Maybe<FrameObject*> FrameObjectBuilder::BuildFastPath(
    Handle<PyFunction> func,
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
  LoadPosActualArgsToLocalsplus(ctx, actual_args,
                                actual_arg_cnt - actual_kw_arg_cnt);

  Handle<PyDict> kw_args;
  if (!AssignKwArgsFromActualArgs(ctx, actual_args, kwarg_keys, kw_args)) {
    return kNullMaybe;
  }

  FillDefaultArgs(ctx, func->default_args());
  if (!CheckMissingArgs(ctx)) {
    return kNullMaybe;
  }

  Handle<PyTuple> extend_pos_args;
  if (!PackExtraPosArgsFromActualArgs(ctx, actual_args, actual_kw_arg_cnt,
                                      extend_pos_args)) {
    return kNullMaybe;
  }

  // 注入 *args/**kwargs。
  InjectVarArgsAndKwArgs(ctx, extend_pos_args, kw_args);

  auto* frame_object = FrameObject::Create(
      *ctx.code_object->consts(), *ctx.code_object->names(), *ctx.locals,
      *ctx.globals, *ctx.localsplus, *ctx.stack, *ctx.code_object, *ctx.func);
  return Maybe<FrameObject*>(frame_object);
}

}  // namespace saauso::internal
