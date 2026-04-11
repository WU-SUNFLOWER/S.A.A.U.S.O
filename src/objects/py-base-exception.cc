// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception.h"

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/native-layout-kind.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

// static
Tagged<PyBaseException> PyBaseException::cast(Tagged<PyObject> object) {
  assert(IsPyBaseException(object));
  return Tagged<PyBaseException>::cast(object);
}

Handle<PyTuple> PyBaseException::args(Isolate* isolate) const {
  if (args_.is_null()) {
    return Handle<PyTuple>::null();
  }
  return handle(Tagged<PyTuple>::cast(args_), isolate);
}

void PyBaseException::set_args(Handle<PyTuple> args) {
  set_args(*args);
}

void PyBaseException::set_args(Tagged<PyTuple> args) {
  args_ = args;
  WRITE_BARRIER(Tagged<PyObject>(this), &args_, args);
}

}  // namespace saauso::internal
