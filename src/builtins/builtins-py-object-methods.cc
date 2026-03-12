// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-object-methods.h"

#include <cinttypes>

#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

Maybe<void> PyObjectBuiltinMethods::Install(Isolate* isolate,
                                            Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_OBJECT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyObjectBuiltinMethods, Init) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> instance;
  Handle<PyObject> init_args = args;

  if (!self.is_null()) {
    instance = self;
  } else {
    int64_t argc = args.is_null() ? 0 : args->length();
    if (argc == 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "descriptor '__init__' of 'object' object needs an "
                         "argument");
      return kNullMaybeHandle;
    }
    instance = args->Get(0);
    if (argc == 1) {
      init_args = Handle<PyTuple>::null();
    } else {
      Handle<PyTuple> tail = PyTuple::NewInstance(argc - 1);
      for (int64_t i = 1; i < argc; ++i) {
        tail->SetInternal(i - 1, *args->Get(i));
      }
      init_args = tail;
    }
  }

  return PyObjectKlass::GetInstance()->InitInstance(isolate, instance,
                                                    init_args, kwargs);
}

}  // namespace saauso::internal
