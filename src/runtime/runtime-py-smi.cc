// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-smi.h"

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

MaybeHandle<PyObject> Runtime_NewSmi(Handle<PyObject> args,
                                     Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "int() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc == 0) {
    return scope.Escape(handle(PySmi::FromInt(0)));
  }
  if (argc > 2) {
    Runtime_ThrowTypeErrorf("int() takes at most 2 arguments (%lld given)",
                            static_cast<long long>(argc));
    return kNullMaybeHandle;
  }

  Handle<PyObject> value = pos_args->Get(0);

  if (argc == 2) {
    if (!IsPyString(value)) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "int() can't convert non-string with explicit base");
      return kNullMaybeHandle;
    }

    int64_t base = -1;
    ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), base,
                               Runtime_DecodeIntLike(pos_args->GetTagged(1)));

    if (!(base == 0 || (2 <= base && base <= 36))) {
      Runtime_ThrowError(ExceptionType::kValueError,
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
        Runtime_ThrowError(ExceptionType::kValueError,
                           "int too large to convert to Smi");
        return kNullMaybeHandle;
      }
      if (err == StringToIntError::kInvalidBase) {
        Runtime_ThrowError(ExceptionType::kValueError,
                           "int() base must be >= 2 and <= 36, or 0");
        return kNullMaybeHandle;
      }
      Runtime_ThrowError(ExceptionType::kValueError,
                         "invalid literal for int() with explicit base");
      return kNullMaybeHandle;
    }

    if (!InRangeWithRightClose(parsed_value, PySmi::kSmiMinValue,
                               PySmi::kSmiMaxValue)) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "int too large to convert to Smi");
      return kNullMaybeHandle;
    }
    return scope.Escape(handle(PySmi::FromInt(parsed_value)));
  }

  if (IsPySmi(value)) {
    return scope.Escape(value);
  }
  if (IsPyBoolean(value)) {
    bool v = Tagged<PyBoolean>::cast(*value)->value();
    return scope.Escape(handle(PySmi::FromInt(v ? 1 : 0)));
  }
  if (IsPyFloat(value)) {
    double v = Handle<PyFloat>::cast(value)->value();
    if (std::isnan(v)) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "cannot convert float NaN to integer");
      return kNullMaybeHandle;
    }
    if (std::isinf(v)) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "cannot convert float infinity to integer");
      return kNullMaybeHandle;
    }

    double truncated = std::trunc(v);
    if (truncated < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
        truncated > static_cast<double>(std::numeric_limits<int64_t>::max())) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "int too large to convert to Smi");
      return kNullMaybeHandle;
    }
    int64_t int_value = static_cast<int64_t>(truncated);
    if (!InRangeWithRightClose(int_value, PySmi::kSmiMinValue,
                               PySmi::kSmiMaxValue)) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "int too large to convert to Smi");
      return kNullMaybeHandle;
    }
    return scope.Escape(handle(PySmi::FromInt(int_value)));
  }
  if (IsPyString(value)) {
    return scope.Escape(handle(PySmi::FromPyString(PyString::cast(*value))));
  }

  auto type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowTypeErrorf(
      "int() argument must be a string, a bytes-like object or a real number, "
      "not '%s'",
      type_name->buffer());
  return kNullMaybeHandle;
}

}  // namespace saauso::internal
