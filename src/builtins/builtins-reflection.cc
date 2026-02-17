// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime.h"

namespace saauso::internal {

BUILTIN(Len) {
  return PyObject::Len(args->Get(0));
}

BUILTIN(IsInstance) {
  EscapableHandleScope scope;

  const auto args_length = args.is_null() ? 0 : args->length();
  if (args_length != 2) [[unlikely]] {
    std::fprintf(stderr, "TypeError: isinstance expected 2 arguments\n");
    std::exit(1);
  }

  auto object = args->Get(0);
  auto type_or_tuple = args->Get(1);
  assert(!type_or_tuple.is_null());

  bool matched = false;
  if (IsPyTypeObject(type_or_tuple)) {
    matched = Runtime_IsInstanceOfTypeObject(
        object, Handle<PyTypeObject>::cast(type_or_tuple));
  } else if (IsPyTuple(type_or_tuple)) {
    auto tuple = Handle<PyTuple>::cast(type_or_tuple);
    for (auto i = 0; i < tuple->length(); ++i) {
      auto elem = tuple->Get(i);
      if (!IsPyTypeObject(elem)) [[unlikely]] {
        std::fprintf(stderr, "TypeError: isinstance() arg 2 must be a type\n");
        std::exit(1);
      }
      if (Runtime_IsInstanceOfTypeObject(object,
                                         Handle<PyTypeObject>::cast(elem))) {
        matched = true;
        break;
      }
    }
  } else {
    std::fprintf(stderr, "TypeError: isinstance() arg 2 must be a type\n");
    std::exit(1);
  }

  auto result = handle(Isolate::ToPyBoolean(matched));
  return scope.Escape(result);
}

BUILTIN(BuildTypeObject) {
  EscapableHandleScope scope;

  const auto args_length = args.is_null() ? 0 : args->length();
  if (args_length < 2) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: not enough arguments");
    std::exit(1);
  }

  auto class_builder_obj = args->Get(0);
  if (!IsPyFunction(class_builder_obj)) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: func is not a function");
    std::exit(1);
  }
  auto class_builder = Handle<PyFunction>::cast(class_builder_obj);

  auto class_name_obj = args->Get(1);
  if (!IsPyString(class_name_obj)) [[unlikely]] {
    std::fprintf(stderr, "__build_class__: name is not a string");
    std::exit(1);
  }
  auto class_name = Handle<PyString>::cast(class_name_obj);

  auto class_supers = PyList::NewInstance(args_length - 2);
  for (auto i = 2; i < args_length; ++i) {
    PyList::Append(class_supers, args->Get(i));
  }

  auto class_properties = PyDict::NewInstance();
  Isolate::Current()->interpreter()->CallPython(
      class_builder, Handle<PyTuple>::null(), Handle<PyTuple>::null(),
      Handle<PyDict>::null(), class_properties);

  Handle<PyTypeObject> type_object =
      Runtime_CreatePythonClass(class_name, class_properties, class_supers);
  return scope.Escape(type_object);
}

}  // namespace saauso::internal
