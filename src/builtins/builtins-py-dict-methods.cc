// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-methods.h"

#include <cinttypes>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

void PyDictBuiltinMethods::Install(Handle<PyDict> target) {
  PY_DICT_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyDictBuiltinMethods, SetDefault) {
  EscapableHandleScope scope;

  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);

  auto value = dict->Get(key);
  if (!value.is_null()) {
    return scope.Escape(value);
  }

  value = handle(Isolate::Current()->py_none_object());
  if (args->length() > 1) {
    value = args->Get(1);
  }

  PyDict::Put(dict, key, value);

  return scope.Escape(value);
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
  auto value = dict->Get(key);
  if (!value.is_null()) {
    dict->Remove(key);
    return scope.Escape(value);
  }

  if (args->length() == 2) {
    return scope.Escape(args->Get(1));
  }

  // 对齐 CPython：dict.pop() 在 key 不存在且无默认值时抛出 KeyError。
  // TODO: 当 repr 机制完善后，改为携带 key 的 repr 信息。
  Runtime_ThrowError(ExceptionType::kKeyError, nullptr);
  return kNullMaybeHandle;
}

BUILTIN_METHOD(PyDictBuiltinMethods, Keys) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictKeys::NewInstance(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Values) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictValues::NewInstance(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Items) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictItems::NewInstance(self));
}

BUILTIN_METHOD(PyDictBuiltinMethods, Get) {
  EscapableHandleScope scope;
  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);

  Handle<PyObject> result = dict->Get(key);
  if (result.is_null()) {
    result = handle(Isolate::Current()->py_none_object());
  }

  return scope.Escape(result);
}

}  // namespace saauso::internal
