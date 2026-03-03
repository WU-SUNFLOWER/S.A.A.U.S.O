// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-tuple-iterator-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

Maybe<void> PyTupleIteratorBuiltinMethods::Install(Isolate* isolate,
                                                   Handle<PyDict> target) {
  PY_TUPLE_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTupleIteratorBuiltinMethods, Next) {
  return PyObject::Next(self);
}

}  // namespace saauso::internal
