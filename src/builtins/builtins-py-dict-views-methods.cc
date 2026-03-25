// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-views-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

Maybe<void> PyDictKeyIteratorBuiltinMethods::Install(
    Isolate* isolate,
    Handle<PyDict> target,
    Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

Maybe<void> PyDictItemIteratorBuiltinMethods::Install(
    Isolate* isolate,
    Handle<PyDict> target,
    Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

Maybe<void> PyDictValueIteratorBuiltinMethods::Install(
    Isolate* isolate,
    Handle<PyDict> target,
    Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_DICT_ITERATOR_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyDictKeyIteratorBuiltinMethods, Next) {
  return PyObject::Next(isolate, self);
}

BUILTIN_METHOD(PyDictItemIteratorBuiltinMethods, Next) {
  return PyObject::Next(isolate, self);
}

BUILTIN_METHOD(PyDictValueIteratorBuiltinMethods, Next) {
  return PyObject::Next(isolate, self);
}

}  // namespace saauso::internal
