// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception.h"

#include "src/execution/isolate.h"
#include "src/objects/py-base-exception-klass.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

// static
Tagged<PyBaseException> PyBaseException::cast(Tagged<PyObject> object) {
  assert(PyObject::GetHeapKlassUnchecked(object) ==
         PyBaseExceptionKlass::GetInstance(Isolate::Current()));
  return Tagged<PyBaseException>::cast(object);
}

}  // namespace saauso::internal
