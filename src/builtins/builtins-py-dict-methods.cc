// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-methods.h"

#include <cinttypes>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
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
