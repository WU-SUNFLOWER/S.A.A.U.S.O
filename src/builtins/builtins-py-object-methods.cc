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

namespace saauso::internal {

Maybe<void> PyObjectBuiltinMethods::Install(Isolate* isolate,
                                            Handle<PyDict> target,
                                            Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name)                 \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name, \
                              owner_type)

  PY_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyObjectBuiltinMethods, New) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> type_object;
  Handle<PyObject> new_args = args;

  if (!self.is_null()) {
    type_object = self;
  } else {
    int64_t argc = args.is_null() ? 0 : args->length();
    if (argc == 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "descriptor '__new__' of 'object' object needs an "
                         "argument");
      return kNullMaybeHandle;
    }
    type_object = args->Get(0);
    if (argc == 1) {
      new_args = Handle<PyTuple>::null();
    } else {
      Handle<PyTuple> tail = PyTuple::NewInstance(argc - 1);
      for (int64_t i = 1; i < argc; ++i) {
        tail->SetInternal(i - 1, *args->Get(i));
      }
      new_args = tail;
    }
  }

  if (!IsPyTypeObject(type_object)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "object.__new__() argument 1 must be type, not '%s'",
                        PyObject::GetKlass(type_object)->name()->buffer());
    return kNullMaybeHandle;
  }

  return PyObjectKlass::GetInstance()->NewInstance(
      isolate, Handle<PyTypeObject>::cast(type_object), new_args, kwargs);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Init) {
  auto* isolate = Isolate::Current();
  return PyObjectKlass::GetInstance()->InitInstance(isolate, self, args,
                                                    kwargs);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Repr) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "object.__repr__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Repr(self);
}

BUILTIN_METHOD(PyObjectBuiltinMethods, Str) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "object.__str__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Str(self);
}

}  // namespace saauso::internal
