// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-type-object.h"

#include "src/execution/exception-utils.h"
#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"

namespace saauso::internal {

MaybeHandle<PyObject> Runtime_NewType(Isolate* isolate,
                                      Handle<PyObject> args,
                                      Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (!(argc == 1 || argc == 3)) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "type() takes 1 or 3 arguments");
    return kNullMaybeHandle;
  }

  if (argc == 1) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                         "type() takes 1 or 3 arguments");
      return kNullMaybeHandle;
    }

    Handle<PyObject> obj = pos_args->Get(0);
    return scope.Escape(PyObject::ResolveObjectKlass(obj, isolate)
                            ->type_object());
  }

  Handle<PyObject> name_obj = pos_args->Get(0);
  Handle<PyObject> bases_obj = pos_args->Get(1);
  Handle<PyObject> dict_obj = pos_args->Get(2);

  if (!IsPyString(name_obj)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "type() argument 1 must be str, not '%s'",
                        PyObject::GetTypeName(name_obj, isolate)->buffer());
    return kNullMaybeHandle;
  }
  if (!IsPyTuple(bases_obj)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "type() argument 2 must be tuple, not '%s'",
                        PyObject::GetTypeName(bases_obj, isolate)->buffer());
    return kNullMaybeHandle;
  }
  if (!IsPyDict(dict_obj)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "type() argument 3 must be dict, not '%s'",
                        PyObject::GetTypeName(dict_obj, isolate)->buffer());
    return kNullMaybeHandle;
  }

  Handle<PyString> name = Handle<PyString>::cast(name_obj);
  Handle<PyTuple> bases_tuple = Handle<PyTuple>::cast(bases_obj);
  Handle<PyDict> class_dict =
      isolate->factory()->CopyPyDict(Handle<PyDict>::cast(dict_obj));

  Handle<PyList> supers;
  if (bases_tuple->length() > 0) {
    supers = PyList::New(isolate, bases_tuple->length());
    for (int64_t i = 0; i < bases_tuple->length(); ++i) {
      Handle<PyObject> base = bases_tuple->Get(i);
      if (!IsPyTypeObject(base)) {
        Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                           "type() bases must be types");
        return kNullMaybeHandle;
      }
      PyList::Append(supers, base, isolate);
    }
  }

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Runtime_CreatePythonClass(isolate, name, class_dict, supers));
  return scope.Escape(result);
}

MaybeHandle<PyString> Runtime_NewTypeObjectRepr(
    Isolate* isolate,
    Handle<PyTypeObject> type_object) {
  EscapableHandleScope scope;

  Handle<PyString> type_name = type_object->own_klass()->name();
  std::string repr = "<class '";
  repr.append(type_name->ToStdString());
  repr.append("'>");

  return scope.Escape(PyString::FromStdString(isolate, repr));
}

}  // namespace saauso::internal
