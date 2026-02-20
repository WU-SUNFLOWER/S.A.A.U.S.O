// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-state.h"

#include "src/objects/py-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

Handle<PyObject> ExceptionState::pending_exception() const {
  return handle(pending_exception_);
}

void ExceptionState::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&pending_exception_);
}

}  // namespace saauso::internal
