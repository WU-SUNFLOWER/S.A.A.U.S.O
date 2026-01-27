// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/frame-object.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdlib>

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/visitors.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// 仅用于创建根栈帧
FrameObject* FrameObject::NewInstance(Handle<PyCodeObject> code_object) {
  HandleScope scope;

  Handle<PyDict> locals = PyDict::NewInstance();
  Handle<FixedArray> fast_locals;
  if (code_object->nlocals() > 0) {
    fast_locals = FixedArray::NewInstance(code_object->nlocals());
  }
  Handle<FixedArray> stack = FixedArray::NewInstance(code_object->stack_size_);

  return NewInstanceImpl(code_object->consts_, code_object->names_, *locals,
                         *locals, *fast_locals, *stack, *code_object);
}

// 创建一般的python栈帧
FrameObject* FrameObject::NewInstance(Handle<PyFunction> func,
                                      Handle<PyObject> host,
                                      Handle<PyTuple> actual_pos_args,
                                      Handle<PyDict> actual_kw_args) {
  HandleScope scope;

  Handle<PyCodeObject> code_object = func->func_code();
  Handle<PyDict> globals = func->func_globals();
  Handle<PyDict> locals = PyDict::NewInstance();
  Handle<FixedArray> fast_locals;
  if (code_object->nlocals() > 0) {
    fast_locals = FixedArray::NewInstance(code_object->nlocals());
  }
  Handle<FixedArray> stack = FixedArray::NewInstance(code_object->stack_size_);

  Handle<PyString> func_name = code_object->co_name();

  int fast_locals_idx = 0;
  int self_arg_cnt = 0;
  // 将host填充为首个函数参数
  if (!host.IsNull()) {
    fast_locals->Set(fast_locals_idx++, host);
    self_arg_cnt = 1;
  }

  // 不算*args和**kwargs的形参个数
  const int64_t real_formal_pos_arg_cnt = code_object->arg_count();
  // 不算self、*args和**kwargs的形参个数
  const int64_t formal_pos_arg_cnt = real_formal_pos_arg_cnt - self_arg_cnt;

  // 将与形参对应的函数实参加载到栈帧的fast_locals上去
  if (!actual_pos_args.IsNull()) {
    auto valid_pos_args_cnt =
        std::min(actual_pos_args->length(), formal_pos_arg_cnt);
    for (auto i = 0; i < valid_pos_args_cnt; ++i) {
      fast_locals->Set(fast_locals_idx++, actual_pos_args->Get(i));
    }
  }

  Handle<PyDict> kw_args;
  // 尝试使用actual_kw_args填充没有通过位置实参赋值的形参
  if (!actual_kw_args.IsNull()) {
    if (code_object->flags() & PyCodeObject::Flag::kVarKeywords) {
      kw_args = PyDict::Clone(actual_kw_args);
    }

    // 遍历actual_kw_args，看看是否是对函数的形参进行赋值
    // 1. 如果是，那么注入到形参
    // 2. 如果不是
    //   - 如果函数有kwarg标志位，那么整理到kw_args当中去
    //   - 如果函数没有kwarg标志位，抛出错误
    auto iter = PyDictItemIterator::NewInstance(actual_kw_args);
    auto item = Handle<PyTuple>::cast(PyObject::Next(iter));
    auto var_args = code_object->var_names();
    while (!item.IsNull()) {
      auto key = Handle<PyString>::cast(item->Get(0));
      auto value = item->Get(1);
      auto index_in_var_args = var_args->IndexOf(key);
      // 如果当前key对应于形参列表中的某个形参
      if (InRangeWithRightOpen(index_in_var_args, static_cast<int64_t>(0),
                               real_formal_pos_arg_cnt)) {
        // 不允许重复对形参赋值
        if (!fast_locals->Get(index_in_var_args).IsNull()) {
          std::printf("TypeError: %s() got multiple values for argument '%s'\n",
                      func_name->buffer(), key->buffer());
          PyObject::Print(value);
          std::exit(1);
        }

        // 给形参赋值
        fast_locals->Set(index_in_var_args, value);
        // 从kw_args中消去该键值对
        if (!kw_args.IsNull()) {
          kw_args->Remove(key);
        }
        // 现在有新的形参被填充进fast_locals了，
        // 因此让fast_locals_idx游标在逻辑上向右移动一下
        ++fast_locals_idx;
      }

      // 刷新迭代器
      item = Handle<PyTuple>::cast(PyObject::Next(iter));
    }
  }

  // 使用默认值填装尚未被赋值的函数形参
  auto default_args = func->default_args();
  if (!default_args.IsNull()) {
    auto default_arg_cnt = default_args->length();
    auto arg_list_index = real_formal_pos_arg_cnt - 1;
    while (default_arg_cnt > 0 && fast_locals->Get(arg_list_index).IsNull()) {
      fast_locals->Set(arg_list_index, default_args->Get(default_arg_cnt - 1));
      --default_arg_cnt;
      --arg_list_index;
      // 同理，让fast_locals_idx游标在逻辑上向右移动一下
      ++fast_locals_idx;
    }
  }

  // 如果还有形参没有有效值，那么抛出错误
  if (fast_locals_idx < real_formal_pos_arg_cnt) {
    std::printf("TypeError: %s() missing %" PRId64
                " required positional argument",
                func_name->buffer(), real_formal_pos_arg_cnt - fast_locals_idx);
    std::exit(1);
  }

  // 打包拓展参数
  Handle<PyTuple> extend_pos_args;
  if (!actual_pos_args.IsNull()) {
    // 用户实际传入的拓展参数个数
    auto extend_pos_arg_cnt = actual_pos_args->length() - formal_pos_arg_cnt;
    if (extend_pos_arg_cnt > 0) {
      // 如果函数不接受扩展参数，直接报错
      if (!(code_object->flags() & PyCodeObject::Flag::kVarArgs)) {
        std::printf("TypeError: %s() takes %" PRId64
                    " positional arguments but %" PRId64 " were given",
                    func_name->buffer(), real_formal_pos_arg_cnt,
                    actual_pos_args->length() + self_arg_cnt);
        std::exit(1);
      }

      extend_pos_args = PyTuple::NewInstance(extend_pos_arg_cnt);

      if (extend_pos_arg_cnt > 0) {
        auto extend_pos_args_idx = 0;
        for (auto i = formal_pos_arg_cnt; i < actual_pos_args->length(); ++i) {
          extend_pos_args->SetInternal(extend_pos_args_idx,
                                       actual_pos_args->Get(i));
          ++extend_pos_args_idx;
        }
      }
    }
  }

  // 向fast_locals中注入extend_pos_args
  if (!extend_pos_args.IsNull()) {
    fast_locals->Set(fast_locals_idx++, extend_pos_args);
  }

  // 向fast_locals中注入kw_args
  if (!kw_args.IsNull()) {
    fast_locals->Set(fast_locals_idx++, kw_args);
  }

  return NewInstanceImpl(code_object->consts_, code_object->names_, *locals,
                         *globals, *fast_locals, *stack, *code_object);
}

// 本函数当中严禁任何可能触发GC的操作!!!
FrameObject* FrameObject::NewInstanceImpl(Tagged<PyObject> consts,
                                          Tagged<PyObject> names,
                                          Tagged<PyObject> locals,
                                          Tagged<PyObject> globals,
                                          Tagged<PyObject> fast_locals,
                                          Tagged<PyObject> stack,
                                          Tagged<PyObject> code_object) {
  FrameObject* frame = new FrameObject();

  frame->consts_ = consts;
  frame->names_ = names;

  frame->locals_ = locals;
  frame->globals_ = globals;
  frame->fast_locals_ = fast_locals;

  frame->stack_ = stack;

  frame->code_object_ = code_object;
  frame->pc_ = 0;
  frame->caller_ = nullptr;
  frame->is_entry_frame_ = false;

  return frame;
}

FrameObject::~FrameObject() {}

int FrameObject::StackSize() const {
  return stack_top_;
}

Handle<PyObject> FrameObject::TopOfStack() const {
  assert(0 < stack_top_);
  return handle(stack()->Get(stack_top_ - 1));
}

Tagged<PyObject> FrameObject::TopOfStackTagged() const {
  assert(0 < stack_top_);
  return Tagged<FixedArray>::cast(stack_)->Get(stack_top_ - 1);
}

void FrameObject::PushToStack(Handle<PyObject> object) {
  PushToStack(*object);
}

void FrameObject::PushToStack(Tagged<PyObject> object) {
  assert(stack_top_ < stack()->capacity());
  stack()->Set(stack_top_++, object);
}

Handle<PyObject> FrameObject::PopFromStack() {
  assert(stack_top_ > 0);
  return handle(stack()->Get(--stack_top_));
}

Tagged<PyObject> FrameObject::PopFromStackTagged() {
  assert(stack_top_ > 0);
  return Tagged<FixedArray>::cast(stack_)->Get(--stack_top_);
}

Tagged<PyObject> FrameObject::StackGetTagged(int index) const {
  return Tagged<FixedArray>::cast(stack_)->Get(index);
}

Handle<FixedArray> FrameObject::stack() const {
  return handle(Tagged<FixedArray>::cast(stack_));
}

Handle<PyTuple> FrameObject::consts() const {
  return handle(Tagged<PyTuple>::cast(consts_));
}

Handle<PyTuple> FrameObject::names() const {
  return handle(Tagged<PyTuple>::cast(names_));
}

Handle<PyDict> FrameObject::locals() const {
  return handle(Tagged<PyDict>::cast(locals_));
}

Handle<FixedArray> FrameObject::fast_locals() const {
  return handle(Tagged<FixedArray>::cast(fast_locals_));
}

Handle<PyDict> FrameObject::globals() const {
  return handle(Tagged<PyDict>::cast(globals_));
}

Handle<PyCodeObject> FrameObject::code_object() const {
  return handle(Tagged<PyCodeObject>::cast(code_object_));
}

int FrameObject::GetOpArg() {
  return code_object()->bytecodes()->buffer()[pc_++] & kByteMask;
}

uint8_t FrameObject::GetOpCode() {
  return code_object()->bytecodes()->buffer()[pc_++];
}

bool FrameObject::HasMoreCodes() {
  return pc_ < code_object()->bytecodes()->length();
}

void FrameObject::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&stack_);
  v->VisitPointer(&consts_);
  v->VisitPointer(&names_);
  v->VisitPointer(&locals_);
  v->VisitPointer(&fast_locals_);
  v->VisitPointer(&globals_);
  v->VisitPointer(&code_object_);
}

}  // namespace saauso::internal
