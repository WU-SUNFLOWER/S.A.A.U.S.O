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
#include "src/objects/visitors.h"

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
                                      Handle<PyTuple> args) {
  HandleScope scope;

  Handle<PyCodeObject> code_object = func->func_code();
  Handle<PyDict> globals = func->func_globals();
  Handle<PyDict> locals = PyDict::NewInstance();
  Handle<FixedArray> fast_locals;
  if (code_object->nlocals() > 0) {
    fast_locals = FixedArray::NewInstance(code_object->nlocals());
  }
  Handle<FixedArray> stack = FixedArray::NewInstance(code_object->stack_size_);

  int fast_locals_idx = 0;
  // 将host填充为首个函数参数
  if (!host.IsNull()) {
    fast_locals->Set(fast_locals_idx++, host);
  }

  // 将函数参数加载到栈帧的fast_locals上去
  if (!args.IsNull()) {
    assert(args->length() <= fast_locals->capacity());
    for (auto i = 0; i < args->length(); ++i) {
      fast_locals->Set(fast_locals_idx++, args->Get(i));
    }
  }

  // 使用默认值填装函数参数
  if (!func->default_args().IsNull()) {
    auto default_args = func->default_args();
    auto default_arg_cnt = default_args->length();
    auto arg_list_index = func->func_code()->arg_count() - 1;
    while (default_arg_cnt > 0 && fast_locals->Get(arg_list_index).IsNull()) {
      fast_locals->Set(arg_list_index, default_args->Get(default_arg_cnt - 1));
      --default_arg_cnt;
      --arg_list_index;
    }
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
