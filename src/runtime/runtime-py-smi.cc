// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-smi.h"

#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-conversions.h"
#include "src/utils/number-conversion.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Handle<PyObject> Runtime_NewSmi(Handle<PyObject> args,
                                Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    std::fprintf(stderr, "TypeError: int() takes no keyword arguments\n");
    std::exit(1);
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc == 0) {
    return scope.Escape(handle(PySmi::FromInt(0)));
  }
  if (argc > 2) {
    std::fprintf(stderr,
                 "TypeError: int() takes at most 2 arguments (%lld given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  Handle<PyObject> value = pos_args->Get(0);

  if (argc == 2) {
    if (!IsPyString(value)) {
      std::fprintf(
          stderr,
          "TypeError: int() can't convert non-string with explicit base\n");
      std::exit(1);
    }

    Maybe<int64_t> maybe_base = Runtime_DecodeIntLike(pos_args->GetTagged(1));
    if (maybe_base.IsNothing()) {
      return Handle<PyObject>::null();
    }
    int64_t base = maybe_base.ToChecked();
    if (!(base == 0 || (2 <= base && base <= 36))) {
      std::fprintf(stderr,
                   "ValueError: int() base must be >= 2 and <= 36, or 0\n");
      std::exit(1);
    }

    auto s = Handle<PyString>::cast(value);
    std::string_view text(s->buffer(), static_cast<size_t>(s->length()));
    int64_t parsed_value = 0;
    StringToIntError err =
        StringToIntWithBase(text, static_cast<int>(base), &parsed_value);
    if (err != StringToIntError::kOk) {
      if (err == StringToIntError::kOverflow) {
        std::fprintf(stderr,
                     "OverflowError: int too large to convert to Smi\n");
        std::exit(1);
      }
      if (err == StringToIntError::kInvalidBase) {
        std::fprintf(stderr,
                     "ValueError: int() base must be >= 2 and <= 36, or 0\n");
        std::exit(1);
      }
      std::fprintf(
          stderr,
          "ValueError: invalid literal for int() with base %lld: '%.*s'\n",
          static_cast<long long>(base), static_cast<int>(s->length()),
          s->buffer());
      std::exit(1);
    }

    if (!InRangeWithRightClose(parsed_value, PySmi::kSmiMinValue,
                               PySmi::kSmiMaxValue)) {
      std::fprintf(stderr, "OverflowError: int too large to convert to Smi\n");
      std::exit(1);
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
      std::fprintf(stderr, "ValueError: cannot convert float NaN to integer\n");
      std::exit(1);
    }
    if (std::isinf(v)) {
      std::fprintf(stderr,
                   "OverflowError: cannot convert float infinity to integer\n");
      std::exit(1);
    }

    double truncated = std::trunc(v);
    if (truncated < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
        truncated > static_cast<double>(std::numeric_limits<int64_t>::max())) {
      std::fprintf(stderr, "OverflowError: int too large to convert to Smi\n");
      std::exit(1);
    }
    int64_t int_value = static_cast<int64_t>(truncated);
    if (!InRangeWithRightClose(int_value, PySmi::kSmiMinValue,
                               PySmi::kSmiMaxValue)) {
      std::fprintf(stderr, "OverflowError: int too large to convert to Smi\n");
      std::exit(1);
    }
    return scope.Escape(handle(PySmi::FromInt(int_value)));
  }
  if (IsPyString(value)) {
    return scope.Escape(handle(PySmi::FromPyString(PyString::cast(*value))));
  }

  auto type_name = PyObject::GetKlass(value)->name();
  std::fprintf(
      stderr,
      "TypeError: int() argument must be a string, a bytes-like object "
      "or a real number, not '%.*s'\n",
      static_cast<int>(type_name->length()), type_name->buffer());
  std::exit(1);
}

}  // namespace saauso::internal
