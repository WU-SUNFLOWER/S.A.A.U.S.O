// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

// 解析 print() 的关键字参数（sep/end/eol）。
// - 成功时返回 true，失败时抛出 TypeError 并返回 false。
// - sep/end 通过引用返回：null 表示未提供。
bool NormalizePrintArgs(Handle<PyDict> kwargs,
                        Handle<PyObject>& sep,
                        Handle<PyObject>& end) {
  if (kwargs.is_null()) {
    return true;
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
      Runtime_ThrowTypeError("keywords must be strings");
      return false;
    }

    if (PyObject::EqualBool(key, sep_key) ||
        PyObject::EqualBool(key, end_key) ||
        PyObject::EqualBool(key, eol_key)) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    Runtime_ThrowTypeErrorf(
        "'print' got an unexpected keyword argument '%.*s'",
        static_cast<int>(key_str->length()), key_str->buffer());
    return false;
  }

  sep = kwargs->Get(sep_key);
  if (!sep.is_null() && !IsPyString(*sep)) {
    auto type_name = PyObject::GetKlass(sep)->name();
    Runtime_ThrowTypeErrorf("sep must be str, not %.*s",
                            static_cast<int>(type_name->length()),
                            type_name->buffer());
    return false;
  }

  end = kwargs->Get(end_key);
  eol = kwargs->Get(eol_key);
  if (!end.is_null() && !eol.is_null()) {
    Runtime_ThrowTypeError(
        "print() got multiple values for keyword argument 'end'");
    return false;
  }

  if (end.is_null()) {
    end = eol;
  }

  if (!end.is_null() && !IsPyString(*end)) {
    auto type_name = PyObject::GetKlass(end)->name();
    Runtime_ThrowTypeErrorf("end must be str, not %.*s",
                            static_cast<int>(type_name->length()),
                            type_name->buffer());
    return false;
  }

  return true;
}

}  // namespace

BUILTIN(Print) {
  EscapableHandleScope scope;

  Handle<PyObject> sep;
  Handle<PyObject> end;
  if (!NormalizePrintArgs(kwargs, sep, end)) {
    return kNullMaybeHandle;
  }

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
