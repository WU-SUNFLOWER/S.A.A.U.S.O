// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/frame-object.h"

#include <cassert>
#include <cstdint>

#include "include/saauso-internal.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

FrameObject* FrameObject::Create(Tagged<PyObject> consts,
                                 Tagged<PyObject> names,
                                 Tagged<PyObject> locals,
                                 Tagged<PyObject> globals,
                                 Tagged<PyObject> localsplus,
                                 Tagged<PyObject> stack,
                                 Tagged<PyObject> code_object,
                                 Tagged<PyObject> func) {
  FrameObject* frame = new FrameObject();

  frame->consts_ = consts;
  frame->names_ = names;

  frame->locals_ = locals;
  frame->globals_ = globals;
  frame->localsplus_ = localsplus;

  frame->stack_ = stack;

  frame->code_object_ = code_object;
  frame->func_ = func;

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

Handle<FixedArray> FrameObject::localsplus() const {
  return handle(Tagged<FixedArray>::cast(localsplus_));
}

Handle<PyDict> FrameObject::globals() const {
  return handle(Tagged<PyDict>::cast(globals_));
}

Handle<PyCodeObject> FrameObject::code_object() const {
  return handle(code_object_tagged());
}

Tagged<PyCodeObject> FrameObject::code_object_tagged() const {
  return Tagged<PyCodeObject>::cast(code_object_);
}

Handle<PyFunction> FrameObject::func() const {
  return handle(func_tagged());
}

Tagged<PyFunction> FrameObject::func_tagged() const {
  return Tagged<PyFunction>::cast(func_);
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
  v->VisitPointer(&localsplus_);
  v->VisitPointer(&globals_);
  v->VisitPointer(&code_object_);
  v->VisitPointer(&func_);
}

}  // namespace saauso::internal
