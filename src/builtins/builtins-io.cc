// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

// 校验 print() 的关键字参数名称：只允许 sep/end/eol。
// 成功时返回 Python None；失败时抛异常并返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> ValidatePrintKeywordArguments(Isolate* isolate,
                                                    Handle<PyDict> kwargs,
                                                    Handle<PyObject> sep_key,
                                                    Handle<PyObject> end_key,
                                                    Handle<PyObject> eol_key) {
  for (auto i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }

    auto key = item->Get(0);
    if (!IsPyString(*key)) {
      Runtime_ThrowError(ExceptionType::kTypeError, "keywords must be strings");
      return kNullMaybeHandle;
    }

    bool eq = false;
    if (!PyObject::EqualBool(key, sep_key).To(&eq)) {
      return kNullMaybeHandle;
    }
    if (eq) {
      continue;
    }

    if (!PyObject::EqualBool(key, end_key).To(&eq)) {
      return kNullMaybeHandle;
    }
    if (eq) {
      continue;
    }

    if (!PyObject::EqualBool(key, eol_key).To(&eq)) {
      return kNullMaybeHandle;
    }
    if (eq) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "'print' got an unexpected keyword argument '%s'",
                        key_str->buffer());
    return kNullMaybeHandle;
  }

  return handle(isolate->py_none_object());
}

// 从 kwargs 中提取 sep/end/eol 的值（未提供则返回 null）。
// 成功时返回 Python None；失败时返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> ExtractPrintKeywordValues(Isolate* isolate,
                                                Handle<PyDict> kwargs,
                                                Handle<PyObject> sep_key,
                                                Handle<PyObject> end_key,
                                                Handle<PyObject> eol_key,
                                                Handle<PyObject>& sep,
                                                Handle<PyObject>& end,
                                                Handle<PyObject>& eol) {
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found, kwargs->Get(sep_key, sep),
                                   kNullMaybeHandle);
  assert(!found || !sep.is_null());

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found, kwargs->Get(end_key, end),
                                   kNullMaybeHandle);
  assert(!found || !end.is_null());

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found, kwargs->Get(eol_key, eol),
                                   kNullMaybeHandle);
  assert(!found || !eol.is_null());

  return handle(isolate->py_none_object());
}

// 校验 sep/end 参数类型，并处理 end/eol 的互斥与兜底赋值规则。
// 成功时返回 Python None；失败时抛异常并返回空 MaybeHandle。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> ValidateAndResolvePrintSeparators(Isolate* isolate,
                                                        Handle<PyObject>& sep,
                                                        Handle<PyObject>& end,
                                                        Handle<PyObject> eol) {
  if (!sep.is_null() && !IsPyString(*sep)) {
    auto type_name = PyObject::GetKlass(sep)->name();
    Runtime_ThrowErrorf(ExceptionType::kTypeError, "sep must be str, not %s",
                        type_name->buffer());
    return kNullMaybeHandle;
  }

  if (!end.is_null() && !eol.is_null()) {
    Runtime_ThrowError(
        ExceptionType::kTypeError,
        "print() got multiple values for keyword argument 'end'");
    return kNullMaybeHandle;
  }

  if (end.is_null()) {
    end = eol;
  }

  if (!end.is_null() && !IsPyString(*end)) {
    auto type_name = PyObject::GetKlass(end)->name();
    Runtime_ThrowErrorf(ExceptionType::kTypeError, "end must be str, not %s",
                        type_name->buffer());
    return kNullMaybeHandle;
  }

  return handle(isolate->py_none_object());
}

// 解析 print() 的关键字参数（sep/end/eol）。
// - 成功时返回 Python None；失败时抛出 TypeError 并返回空 MaybeHandle。
// - sep/end 通过引用返回：null 表示未提供。
// 注意：该函数不能创建独立的 HandleScope，否则会把短生命周期的 handle
// 通过引用返回给调用方，导致悬垂句柄。
MaybeHandle<PyObject> NormalizePrintArgs(Isolate* isolate,
                                         Handle<PyDict> kwargs,
                                         Handle<PyObject>& sep,
                                         Handle<PyObject>& end) {
  if (!kwargs.is_null()) {
    Handle<PyObject> eol;

    auto sep_key = ST(sep);
    auto end_key = ST(end);
    auto eol_key = ST(eol);

    RETURN_ON_EXCEPTION_VALUE(isolate,
                              ValidatePrintKeywordArguments(
                                  isolate, kwargs, sep_key, end_key, eol_key),
                              kNullMaybeHandle);

    RETURN_ON_EXCEPTION_VALUE(
        isolate,
        ExtractPrintKeywordValues(isolate, kwargs, sep_key, end_key, eol_key,
                                  sep, end, eol),
        kNullMaybeHandle);

    RETURN_ON_EXCEPTION_VALUE(
        isolate, ValidateAndResolvePrintSeparators(isolate, sep, end, eol),
        kNullMaybeHandle);
  }

  return handle(isolate->py_none_object());
}

}  // namespace

BUILTIN(Print) {
  EscapableHandleScope scope;
  Isolate* isolate = Isolate::Current();

  Handle<PyObject> sep;
  Handle<PyObject> end;
  RETURN_ON_EXCEPTION_VALUE(
      isolate, NormalizePrintArgs(isolate, kwargs, sep, end), kNullMaybeHandle);

  for (auto i = 0; i < args->length(); ++i) {
    if (i > 0) {
      if (sep.is_null()) {
        std::printf(" ");
      } else {
        if (PyObject::Print(sep).IsEmpty()) {
          return kNullMaybeHandle;
        }
      }
    }
    if (PyObject::Print(args->Get(i)).IsEmpty()) {
      return kNullMaybeHandle;
    }
  }

  if (end.is_null()) {
    std::printf("\n");
  } else {
    if (PyObject::Print(end).IsEmpty()) {
      return kNullMaybeHandle;
    }
  }

  return scope.Escape(handle(isolate->py_none_object()));
}

}  // namespace saauso::internal
