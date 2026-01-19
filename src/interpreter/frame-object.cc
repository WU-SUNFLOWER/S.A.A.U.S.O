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
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

FrameObject::FrameObject(Handle<PyCodeObject> code_object) {
  consts_ = code_object->consts_;
  names_ = code_object->names_;

  locals_ = *PyDict::NewInstance();

  stack_ = *FixedArray::NewInstance(code_object->stack_size_);

  code_object_ = *code_object;
  pc_ = 0;
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

Handle<FixedArray> FrameObject::stack() const {
  return handle(Tagged<FixedArray>::cast(stack_));
}

Handle<PyTuple> FrameObject::consts() const {
  return handle(Tagged<PyTuple>::cast(consts_));
}

Handle<PyTuple> FrameObject::names() const {
  return handle(Tagged<PyTuple>::cast(stack_));
}

Handle<PyDict> FrameObject::locals() const {
  return handle(Tagged<PyDict>::cast(locals_));
}

Handle<PyCodeObject> FrameObject::code_object() const {
  return handle(Tagged<PyCodeObject>::cast(locals_));
}

int FrameObject::GetOpArg() {
  int byte1 = code_object()->bytecodes()->buffer()[pc_++] & kByteMask;
  int byte2 = code_object()->bytecodes()->buffer()[pc_++] & kByteMask;
  return byte2 << kBitsPerByte | byte1;
}

uint8_t FrameObject::GetOpCode() {
  return code_object()->bytecodes()->buffer()[pc_++];
}

bool FrameObject::HasMoreCodes() {
  return pc_ < code_object()->bytecodes()->length();
}

}  // namespace saauso::internal
