// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"
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

BUILTIN(Print) {
  EscapableHandleScope scope;

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

  return scope.Escape(handle(Isolate::Current()->py_none_object()));
}

}  // namespace saauso::internal
