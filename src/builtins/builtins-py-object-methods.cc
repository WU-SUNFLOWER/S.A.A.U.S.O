// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-object-methods.h"

#include <cinttypes>

#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-py-tuple.h"

namespace saauso::internal {

Maybe<void> PyObjectBuiltinMethods::Install(Isolate* isolate,
                                            Handle<PyDict> target,
                                            Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyObjectBuiltinMethods, New) {
  Handle<PyObject> type_object;
  Handle<PyObject> new_args = args;

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc == 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "descriptor '__new__' of 'object' object needs an "
                       "argument");
    return kNullMaybeHandle;
  }
  type_object = args->Get(0, isolate);
  new_args = Runtime_NewTupleTailOrNull(isolate, args, 1);

  if (!IsPyTypeObject(type_object)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "object.__new__() argument 1 must be type, not '%s'",
                        PyObject::GetTypeName(type_object, isolate)->buffer());
    return kNullMaybeHandle;
  }

  return PyObjectKlass::GetInstance(isolate)->NewInstance(
      isolate, Handle<PyTypeObject>::cast(type_object), new_args, kwargs);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Init) {
  return PyObjectKlass::GetInstance(isolate)->InitInstance(isolate, self, args,
                                                           kwargs);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Repr) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "object.__repr__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Repr(isolate, self);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Str) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "object.__str__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Str(isolate, self);
}

}  // namespace saauso::internal
