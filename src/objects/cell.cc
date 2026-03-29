// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/cell.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/cell-klass.h"
#include "src/objects/py-dict.h"

namespace saauso::internal {

// static
Tagged<Cell> Cell::cast(Tagged<PyObject> object) {
  assert(IsCell(object));
  return Tagged<Cell>::cast(object);
}

Handle<PyObject> Cell::value() {
  return handle(value_);
}

Tagged<PyObject> Cell::value_tagged() {
  return value_;
}

void Cell::set_value(Handle<PyObject> value) {
  set_value(*value);
}

void Cell::set_value(Tagged<PyObject> value) {
  value_ = value;
  WRITE_BARRIER(Tagged<PyObject>(this), &value_, value);
}

}  // namespace saauso::internal
