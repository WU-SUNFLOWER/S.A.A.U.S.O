// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-views-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

void PyDictKeyIteratorBuiltinMethods::Install(Handle<PyDict> target) {
  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
}

void PyDictItemIteratorBuiltinMethods::Install(Handle<PyDict> target) {
  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
}

void PyDictValueIteratorBuiltinMethods::Install(Handle<PyDict> target) {
  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyDictKeyIteratorBuiltinMethods, Next) {
  return PyObject::Next(self);
}

BUILTIN_METHOD(PyDictItemIteratorBuiltinMethods, Next) {
  return PyObject::Next(self);
}

BUILTIN_METHOD(PyDictValueIteratorBuiltinMethods, Next) {
  return PyObject::Next(self);
}

}  // namespace saauso::internal
