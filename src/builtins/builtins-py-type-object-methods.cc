// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-type-object-methods.h"

#include <cinttypes>

#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Maybe<void> PyTypeObjectBuiltinMethods::Install(
    Isolate* isolate,
    Handle<PyDict> target,
    Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_TYPE_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Repr) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "type.__repr__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Repr(self);
}

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Str) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "type.__str__() takes no arguments (%" PRId64 " given)",
                        argc);
    return kNullMaybeHandle;
  }
  return PyObject::Str(self);
}

BUILTIN_METHOD(PyTypeObjectBuiltinMethods, Mro) {
  return Handle<PyTypeObject>::cast(self)->mro();
}

}  // namespace saauso::internal
