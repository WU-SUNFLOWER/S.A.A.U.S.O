// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cinttypes>

#include "src/builtins/builtins-utils.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/runtime-truthiness.h"

namespace saauso::internal {

BUILTIN(Len) {
  return PyObject::Len(isolate, args->Get(0));
}

BUILTIN(Repr) {
  return PyObject::Repr(isolate, args->Get(0));
}

BUILTIN(IsInstance) {
  EscapableHandleScope scope;

  const auto args_length = args.is_null() ? 0 : args->length();
  if (args_length != 2) [[unlikely]] {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "isinstance expected 2 arguments, got %" PRId64,
                        static_cast<int64_t>(args_length));
    return kNullMaybeHandle;
  }

  auto object = args->Get(0);
  auto type_or_tuple = args->Get(1);
  assert(!type_or_tuple.is_null());

  bool matched = false;
  if (IsPyTypeObject(type_or_tuple)) {
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, matched,
        Runtime_IsInstanceOfTypeObject(
            isolate, object, Handle<PyTypeObject>::cast(type_or_tuple)));
  } else if (IsPyTuple(type_or_tuple)) {
    auto tuple = Handle<PyTuple>::cast(type_or_tuple);
    for (auto i = 0; i < tuple->length(); ++i) {
      auto elem = tuple->Get(i);
      if (!IsPyTypeObject(elem)) [[unlikely]] {
        Runtime_ThrowError(
            isolate, ExceptionType::kTypeError,
            "isinstance() arg 2 must be a type or tuple of types");
        return kNullMaybeHandle;
      }

      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, matched,
          Runtime_IsInstanceOfTypeObject(isolate, object,
                                         Handle<PyTypeObject>::cast(elem)));

      if (matched) {
        break;
      }
    }
  } else {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "isinstance() arg 2 must be a type or tuple of types");
    return kNullMaybeHandle;
  }

  auto result = handle(Isolate::ToPyBoolean(matched));
  return scope.Escape(result);
}

BUILTIN(BuildTypeObject) {
  EscapableHandleScope scope;

  const auto args_length = args.is_null() ? 0 : args->length();
  if (args_length < 2) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__build_class__: not enough arguments");
    return kNullMaybeHandle;
  }

  auto class_builder_obj = args->Get(0);
  if (!IsPyFunction(class_builder_obj, isolate)) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__build_class__: func must be a function");
    return kNullMaybeHandle;
  }
  auto class_builder = Handle<PyFunction>::cast(class_builder_obj);

  auto class_name_obj = args->Get(1);
  if (!IsPyString(class_name_obj)) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "__build_class__: name is not a string");
    return kNullMaybeHandle;
  }
  auto class_name = Handle<PyString>::cast(class_name_obj);

  auto class_supers = PyList::New(isolate, args_length - 2);
  for (auto i = 2; i < args_length; ++i) {
    PyList::Append(class_supers, args->Get(i), isolate);
  }

  auto class_properties = PyDict::New(isolate);
  if (Execution::Call(isolate, class_builder, Handle<PyTuple>::null(),
                      Handle<PyTuple>::null(), Handle<PyDict>::null(),
                      class_properties)
          .IsEmpty()) {
    return kNullMaybeHandle;
  }

  Handle<PyTypeObject> type_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, type_object,
      Runtime_CreatePythonClass(isolate, class_name, class_properties,
                                class_supers));
  return scope.Escape(type_object);
}

}  // namespace saauso::internal
