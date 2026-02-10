// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/native-functions.h"

#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {
namespace {
void NormalizePrintArgs(Handle<PyDict> kwargs,
                        Handle<PyObject>& sep,
                        Handle<PyObject>& end) {
  if (kwargs.is_null()) {
    return;
  }

  Handle<PyObject> eol;

  auto sep_key = ST(sep);
  auto end_key = ST(end);
  auto eol_key = ST(eol);

  for (auto i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }

    auto key = item->Get(0);
    if (!IsPyString(*key)) {
      std::fprintf(stderr, "TypeError: keywords must be strings\n");
      std::exit(1);
    }

    if (PyObject::EqualBool(key, sep_key) ||
        PyObject::EqualBool(key, end_key) ||
        PyObject::EqualBool(key, eol_key)) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    std::fprintf(
        stderr,
        "TypeError: 'print' got an unexpected keyword argument '%.*s'\n",
        static_cast<int>(key_str->length()), key_str->buffer());
    std::exit(1);
  }

  sep = kwargs->Get(sep_key);
  if (!sep.is_null() && !IsPyString(*sep)) {
    auto type_name = PyObject::GetKlass(sep)->name();
    std::fprintf(stderr, "TypeError: sep must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }

  end = kwargs->Get(end_key);
  eol = kwargs->Get(eol_key);
  if (!end.is_null() && !eol.is_null()) {
    std::fprintf(stderr,
                 "TypeError: print() got multiple values for keyword argument "
                 "'end'\n");
    std::exit(1);
  }

  if (end.is_null()) {
    end = eol;
  }

  if (!end.is_null() && !IsPyString(*end)) {
    auto type_name = PyObject::GetKlass(end)->name();
    std::fprintf(stderr, "TypeError: end must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }
}

}  // namespace

Handle<PyObject> Native_Print(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs) {
  Handle<PyObject> sep;
  Handle<PyObject> end;
  NormalizePrintArgs(kwargs, sep, end);

  for (auto i = 0; i < args->length(); ++i) {
    if (i > 0) {
      if (sep.is_null()) {
        std::printf(" ");
      } else {
        PyObject::Print(sep);
      }
    }
    PyObject::Print(args->Get(i));
  }

  if (end.is_null()) {
    std::printf("\n");
  } else {
    PyObject::Print(end);
  }

  return handle(Isolate::Current()->py_none_object());
}

Handle<PyObject> Native_Len(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs) {
  return PyObject::Len(args->Get(0));
}

Handle<PyObject> Native_IsInstance(Handle<PyObject> host,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs) {
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

Handle<PyObject> Native_BuildTypeObject(Handle<PyObject> host,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs) {
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

Handle<PyObject> Native_Sysgc(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs) {
  Isolate::Current()->heap()->CollectGarbage();
  return handle(Isolate::Current()->py_none_object());
}

}  // namespace saauso::internal
