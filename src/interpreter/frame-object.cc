// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/frame-object.h"

#include <cassert>

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

// 仅用于创建根栈帧
FrameObject::FrameObject(Handle<PyCodeObject> code_object) {
  consts_ = code_object->consts_;
  names_ = code_object->names_;

  locals_ = *PyDict::NewInstance();
  globals_ = locals_;  // 对于根栈帧来说，locals和globals就是同一个变量表

  if (code_object->nlocals() > 0) {
    fast_locals_ = *FixedArray::NewInstance(code_object->nlocals());
  }

  stack_ = *FixedArray::NewInstance(code_object->stack_size_);

  code_object_ = *code_object;
  pc_ = 0;

  caller_ = nullptr;
}

// 创建一般的python栈帧
FrameObject::FrameObject(Handle<PyFunction> func, Handle<PyTuple> args)
    : FrameObject(func->func_code()) {
  HandleScope scope;

  globals_ = *func->func_globals();

  // 加载函数参数
  if (!args.IsNull()) {
    assert(args->length() <= fast_locals()->capacity());
    for (auto i = 0; i < args->length(); ++i) {
      fast_locals()->Set(i, args->Get(i));
    }
  }

  // 使用默认值填装函数参数
  if (!func->default_args().IsNull()) {
    auto default_args = func->default_args();
    auto default_arg_cnt = default_args->length();
    auto arg_list_index = func->func_code()->arg_count() - 1;
    while (default_arg_cnt > 0 && fast_locals()->Get(arg_list_index).IsNull()) {
      fast_locals()->Set(arg_list_index,
                         default_args->Get(default_arg_cnt - 1));
      --default_arg_cnt;
      --arg_list_index;
    }
  }

  return;
}

FrameObject::~FrameObject() {}

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

}  // namespace saauso::internal
