// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-smi.h"

#include <cinttypes>

#include "src/execution/exception-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-conversions.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/number-conversion.h"
#include "src/utils/utils.h"

namespace saauso::internal {

MaybeHandle<PySmi> Runtime_PyStringToSmi(Isolate* isolate,
                                         Tagged<PyString> py_string) {
  EscapableHandleScope scope(isolate);

  std::string_view s(py_string->buffer(),
                     static_cast<size_t>(py_string->length()));

  int64_t parsed = 0;
  StringToIntError err = StringToIntWithBase(s, 10, &parsed);
  if (err != StringToIntError::kOk) {
    if (err == StringToIntError::kOverflow) {
      Runtime_ThrowError(isolate, ExceptionType::kValueError,
                         "int too large to convert to Smi");
    } else {
      Runtime_ThrowErrorf(isolate, ExceptionType::kValueError,
                          "invalid literal for int() with base 10: '%s'\n",
                          py_string->buffer());
    }
    return kNullMaybeHandle;
  }

  if (!InRangeWithRightClose(parsed, PySmi::kSmiMinValue,
                             PySmi::kSmiMaxValue)) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "int too large to convert to Smi");
    return kNullMaybeHandle;
  }

  return scope.Escape(handle(PySmi::FromInt(parsed), isolate));
}

MaybeHandle<PySmi> Runtime_PyFloatToSmi(Isolate* isolate,
                                        Tagged<PyFloat> py_float) {
  EscapableHandleScope scope(isolate);

  double v = py_float->value();
  if (std::isnan(v)) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "cannot convert float NaN to integer");
    return kNullMaybeHandle;
  }

  if (std::isinf(v)) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "cannot convert float infinity to integer");
    return kNullMaybeHandle;
  }

  double truncated = std::trunc(v);
  if (truncated < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
      truncated > static_cast<double>(std::numeric_limits<int64_t>::max())) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "int too large to convert to Smi");
    return kNullMaybeHandle;
  }

  int64_t int_value = static_cast<int64_t>(truncated);
  if (!InRangeWithRightClose(int_value, PySmi::kSmiMinValue,
                             PySmi::kSmiMaxValue)) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "int too large to convert to Smi");
    return kNullMaybeHandle;
  }

  return scope.Escape(handle(PySmi::FromInt(int_value), isolate));
}

MaybeHandle<PySmi> Runtime_NewSmi(Isolate* isolate,
                                  Handle<PyObject> args,
                                  Handle<PyObject> kwargs) {
  EscapableHandleScope scope(isolate);

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "int() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc == 0) {
    return scope.Escape(handle(PySmi::FromInt(0), isolate));
  }
  if (argc > 2) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "int() takes at most 2 arguments (%" PRId64 " given)",
                        argc);
    return kNullMaybeHandle;
  }

  Handle<PyObject> value = pos_args->Get(0, isolate);

  if (argc == 2) {
    if (!IsPyString(value)) {
      Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                         "int() can't convert non-string with explicit base");
      return kNullMaybeHandle;
    }

    int64_t base = -1;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, base, Runtime_DecodeIntLike(isolate, pos_args->GetTagged(1)));

    if (!(base == 0 || (2 <= base && base <= 36))) {
      Runtime_ThrowError(isolate, ExceptionType::kValueError,
                         "int() base must be >= 2 and <= 36, or 0");
      return kNullMaybeHandle;
    }

    auto s = Handle<PyString>::cast(value);
    std::string_view text(s->buffer(), static_cast<size_t>(s->length()));
    int64_t parsed_value = 0;
    StringToIntError err =
        StringToIntWithBase(text, static_cast<int>(base), &parsed_value);
    if (err != StringToIntError::kOk) {
      if (err == StringToIntError::kOverflow) {
        Runtime_ThrowError(isolate, ExceptionType::kValueError,
                           "int too large to convert to Smi");
        return kNullMaybeHandle;
      }
      if (err == StringToIntError::kInvalidBase) {
        Runtime_ThrowError(isolate, ExceptionType::kValueError,
                           "int() base must be >= 2 and <= 36, or 0");
        return kNullMaybeHandle;
      }
      Runtime_ThrowError(isolate, ExceptionType::kValueError,
                         "invalid literal for int() with explicit base");
      return kNullMaybeHandle;
    }

    if (!InRangeWithRightClose(parsed_value, PySmi::kSmiMinValue,
                               PySmi::kSmiMaxValue)) {
      Runtime_ThrowError(isolate, ExceptionType::kValueError,
                         "int too large to convert to Smi");
      return kNullMaybeHandle;
    }
    return scope.Escape(handle(PySmi::FromInt(parsed_value), isolate));
  }

  if (IsPySmi(value)) {
    return scope.Escape(Handle<PySmi>::cast(value));
  }
  if (IsPyBoolean(value)) {
    bool v = Tagged<PyBoolean>::cast(*value)->value();
    return scope.Escape(handle(PySmi::FromInt(v ? 1 : 0), isolate));
  }
  if (IsPyFloat(value)) {
    Handle<PySmi> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Runtime_PyFloatToSmi(isolate, *Handle<PyFloat>::cast(value)));
    return scope.Escape(result);
  }
  if (IsPyString(value)) {
    Handle<PySmi> result;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Runtime_PyStringToSmi(isolate, *Handle<PyString>::cast(value)));
    return scope.Escape(result);
  }

  auto type_name = PyObject::GetTypeName(value, isolate);
  Runtime_ThrowErrorf(
      isolate, ExceptionType::kTypeError,
      "int() argument must be a string, a bytes-like object or a real number, "
      "not '%s'",
      type_name->buffer());
  return kNullMaybeHandle;
}

}  // namespace saauso::internal
