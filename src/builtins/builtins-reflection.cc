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

  auto object = args->Get(0);
  auto mro_of_object = PyObject::GetKlass(object)->mro();
  auto target_type_object = args->Get(1);

  auto result = handle(Isolate::Current()->py_false_object());
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i);
    if (PyObject::EqualBool(curr_type_object, target_type_object)) {
      result = handle(Isolate::Current()->py_true_object());
      break;
    }
  }

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
