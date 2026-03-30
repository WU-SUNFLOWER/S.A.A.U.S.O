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

Handle<PyObject> FrameObject::TopOfStack(Isolate* isolate) const {
  assert(0 < stack_top_);
  return handle(stack(isolate)->Get(stack_top_ - 1), isolate);
}

Tagged<PyObject> FrameObject::TopOfStackTagged() const {
  assert(0 < stack_top_);
  return Tagged<FixedArray>::cast(stack_)->Get(stack_top_ - 1);
}

void FrameObject::PushToStack(Handle<PyObject> object) {
  PushToStack(*object);
}

void FrameObject::PushToStack(Tagged<PyObject> object) {
  assert(stack_top_ < Tagged<FixedArray>::cast(stack_)->capacity());
  Tagged<FixedArray>::cast(stack_)->Set(stack_top_++, object);
}

Handle<PyObject> FrameObject::PopFromStack(Isolate* isolate) {
  assert(stack_top_ > 0);
  return handle(Tagged<FixedArray>::cast(stack_)->Get(--stack_top_), isolate);
}

Tagged<PyObject> FrameObject::PopFromStackTagged() {
  assert(stack_top_ > 0);
  return Tagged<FixedArray>::cast(stack_)->Get(--stack_top_);
}

Tagged<PyObject> FrameObject::StackGetTagged(int index) const {
  return Tagged<FixedArray>::cast(stack_)->Get(index);
}

Handle<FixedArray> FrameObject::stack(Isolate* isolate) const {
  return handle(Tagged<FixedArray>::cast(stack_), isolate);
}

Handle<PyTuple> FrameObject::consts(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(consts_), isolate);
}

Handle<PyTuple> FrameObject::names(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(names_), isolate);
}

Handle<PyDict> FrameObject::locals(Isolate* isolate) const {
  return handle(Tagged<PyDict>::cast(locals_), isolate);
}

Handle<FixedArray> FrameObject::localsplus(Isolate* isolate) const {
  return handle(Tagged<FixedArray>::cast(localsplus_), isolate);
}

Handle<PyDict> FrameObject::globals(Isolate* isolate) const {
  return handle(Tagged<PyDict>::cast(globals_), isolate);
}

Handle<PyCodeObject> FrameObject::code_object(Isolate* isolate) const {
  return handle(code_object_tagged(), isolate);
}

Tagged<PyCodeObject> FrameObject::code_object_tagged() const {
  return Tagged<PyCodeObject>::cast(code_object_);
}

Handle<PyFunction> FrameObject::func(Isolate* isolate) const {
  return handle(func_tagged(), isolate);
}

Tagged<PyFunction> FrameObject::func_tagged() const {
  return Tagged<PyFunction>::cast(func_);
}

int FrameObject::GetOpArg() {
  auto code = Tagged<PyCodeObject>::cast(code_object_);
  auto bytecodes = Tagged<PyString>::cast(code->bytecodes_);
  return bytecodes->buffer()[pc_++] & kByteMask;
}

uint8_t FrameObject::GetOpCode() {
  auto code = Tagged<PyCodeObject>::cast(code_object_);
  auto bytecodes = Tagged<PyString>::cast(code->bytecodes_);
  return bytecodes->buffer()[pc_++];
}

bool FrameObject::HasMoreCodes() {
  auto code = Tagged<PyCodeObject>::cast(code_object_);
  auto bytecodes = Tagged<PyString>::cast(code->bytecodes_);
  return pc_ < bytecodes->length();
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
