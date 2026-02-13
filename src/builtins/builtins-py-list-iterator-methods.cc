// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-list-iterator-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

void PyListIteratorBuiltinMethods::Install(Handle<PyDict> target) {
  PY_LIST_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyListIteratorBuiltinMethods, Next) {
  return PyObject::Next(self);
}

}  // namespace saauso::internal
