// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-methods.h"

#include <cinttypes>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-dict.h"

namespace saauso::internal {

Maybe<void> PyDictBuiltinMethods::Install(Isolate* isolate,
                                          Handle<PyDict> target) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(func_name, method_name) \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, func_name, method_name)

  PY_DICT_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyDictBuiltinMethods, New) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> type_object;
  Handle<PyObject> new_args = args;

  if (!self.is_null()) {
    type_object = self;
  } else {
    int64_t argc = args.is_null() ? 0 : args->length();
    if (argc == 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "descriptor '__new__' of 'dict' object needs an "
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
                        "dict.__new__() argument 1 must be type, not '%s'",
                        PyObject::GetKlass(type_object)->name()->buffer());
    return kNullMaybeHandle;
  }

  return PyDictKlass::GetInstance()->NewInstance(
      isolate, Handle<PyTypeObject>::cast(type_object), new_args, kwargs);
}

BUILTIN_METHOD(PyDictBuiltinMethods, SetDefault) {
  EscapableHandleScope scope;

  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);
  Handle<PyObject> default_or_null = Handle<PyObject>::null();
  if (args->length() > 1) {
    default_or_null = args->Get(1);
  }

  Handle<PyObject> result;
  if (!Runtime_DictSetDefault(dict, key, default_or_null).ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return scope.Escape(result);
}

BUILTIN_METHOD(PyDictBuiltinMethods, Init) {
  auto* isolate = Isolate::Current();

  Handle<PyObject> instance;
  Handle<PyObject> init_args = args;

  if (!self.is_null()) {
    instance = self;
  } else {
    int64_t argc = args.is_null() ? 0 : args->length();
    if (argc == 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "descriptor '__init__' of 'dict' object needs an "
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

  return PyDictKlass::GetInstance()->InitInstance(isolate, instance, init_args,
                                                  kwargs);
}

BUILTIN_METHOD(PyDictBuiltinMethods, Pop) {
  EscapableHandleScope scope;

  if (args->length() < 1 || args->length() > 2) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "pop expected at most 2 arguments, got %" PRId64,
                        args->length());
    return kNullMaybeHandle;
  }

  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);
  bool has_default = args->length() == 2;
  Handle<PyObject> default_or_null =
      has_default ? args->Get(1) : Handle<PyObject>::null();

  Handle<PyObject> result;
  if (!Runtime_DictPop(dict, key, default_or_null, has_default)
           .ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return scope.Escape(result);
}

BUILTIN_METHOD(PyDictBuiltinMethods, Keys) {
  EscapableHandleScope scope;
  return scope.Escape(Isolate::Current()->factory()->NewPyDictKeys(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Values) {
  EscapableHandleScope scope;
  return scope.Escape(Isolate::Current()->factory()->NewPyDictValues(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Items) {
  EscapableHandleScope scope;
  return scope.Escape(Isolate::Current()->factory()->NewPyDictItems(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Get) {
  EscapableHandleScope scope;
  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);
  Handle<PyObject> default_or_null = Handle<PyObject>::null();
  if (args->length() > 1) {
    default_or_null = args->Get(1);
  }

  Handle<PyObject> result;
  if (!Runtime_DictGet(dict, key, default_or_null).ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return scope.Escape(result);
}

}  // namespace saauso::internal
