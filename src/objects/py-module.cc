// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-module.h"

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

// static
Tagged<PyModule> PyModule::cast(Tagged<PyObject> object) {
  assert(IsPyModule(object));
  return Tagged<PyModule>::cast(object);
}

}  // namespace saauso::internal
