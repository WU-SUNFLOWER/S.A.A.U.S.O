// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-state.h"

#include "src/objects/py-base-exception.h"
#include "src/objects/py-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

Tagged<PyBaseException> ExceptionState::pending_exception_tagged() const {
  return Tagged<PyBaseException>::cast(pending_exception_);
}

Handle<PyBaseException> ExceptionState::pending_exception(
    Isolate* isolate) const {
  return handle(pending_exception_tagged(), isolate);
}

void ExceptionState::Throw(Handle<PyBaseException> exception) {
  if (HasPendingException()) {
    return;
  }
  pending_exception_ = *exception;
  pending_exception_pc_ = kInvalidProgramCounter;
  pending_exception_origin_pc_ = kInvalidProgramCounter;
}

void ExceptionState::Clear() {
  pending_exception_ = Tagged<PyObject>::null();
  pending_exception_pc_ = kInvalidProgramCounter;
  pending_exception_origin_pc_ = kInvalidProgramCounter;
}

void ExceptionState::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&pending_exception_);
}

}  // namespace saauso::internal
