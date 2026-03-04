// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-views-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

Maybe<void> PyDictKeyIteratorBuiltinMethods::Install(Isolate* isolate,
                                                     Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

Maybe<void> PyDictItemIteratorBuiltinMethods::Install(Isolate* isolate,
                                                      Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

Maybe<void> PyDictValueIteratorBuiltinMethods::Install(Isolate* isolate,
                                                       Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
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
