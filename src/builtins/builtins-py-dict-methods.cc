// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-dict-methods.h"

#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

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
    std::fprintf(stderr,
                 "TypeError: pop expected at most 2 arguments, got %lld\n",
                 static_cast<long long>(args->length()));
    std::exit(1);
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

  std::printf("KeyError: ");
  PyObject::Print(key);
  std::printf("\n");
  std::exit(1);
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
