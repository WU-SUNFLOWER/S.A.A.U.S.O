// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-type-object-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"

namespace saauso::internal {

void PyTypeObjectBuiltinMethods::Install(Handle<PyDict> target) {
  PY_TYPE_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Mro) {
  return Handle<PyTypeObject>::cast(self)->mro();
}

}  // namespace saauso::internal
