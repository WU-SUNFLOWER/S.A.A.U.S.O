// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-type-object-methods.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Maybe<void> PyTypeObjectBuiltinMethods::Install(Isolate* isolate,
                                                Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_TYPE_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Repr) {
  if (self.is_null()) {
    Runtime_ThrowError(
        ExceptionType::kTypeError,
        "descriptor '__repr__' of 'type' object needs an argument");
    return kNullMaybeHandle;
  }
  return PyObject::Repr(self);
}

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Str) {
  if (self.is_null()) {
    Runtime_ThrowError(
        ExceptionType::kTypeError,
        "descriptor '__str__' of 'type' object needs an argument");
    return kNullMaybeHandle;
  }
  return PyObject::Str(self);
}

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Mro) {
  return Handle<PyTypeObject>::cast(self)->mro();
}

}  // namespace saauso::internal
