// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-float-klass.h"

namespace saauso::internal {

// static
Handle<PyFloat> PyFloat::NewInstance(double value) {
  return Isolate::Current()->factory()->NewPyFloat(value);
}

// static
Tagged<PyFloat> PyFloat::cast(Tagged<PyObject> object) {
  assert(IsPyFloat(object));
  return Tagged<PyFloat>::cast(object);
}

}  // namespace saauso::internal
