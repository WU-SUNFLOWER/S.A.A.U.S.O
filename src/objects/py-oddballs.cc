// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"

namespace saauso::internal {

///////////////////////////////////////////////////////////////////////////
// Python布尔值

// static
Tagged<PyBoolean> PyBoolean::cast(Tagged<PyObject> object) {
  assert(IsPyBoolean(object));
  return Tagged<PyBoolean>::cast(object);
}

///////////////////////////////////////////////////////////////////////////
// Python空值

// static
Tagged<PyNone> PyNone::cast(Tagged<PyObject> object) {
  assert(IsPyNone(object, Isolate::Current()));
  return Tagged<PyNone>::cast(object);
}

}  // namespace saauso::internal
