// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdio>

#include "src/builtins/builtins-utils.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

struct PrintOptions {
  Handle<PyObject> sep;
  Handle<PyObject> end;
  Handle<PyObject> file;
  Handle<PyObject> flush;
};

MaybeHandle<PyObject> NormalizePrintOptions(Isolate* isolate,
                                            Handle<PyDict> kwargs,
                                            PrintOptions& options) {
  if (kwargs.is_null()) {
    return isolate->factory()->py_none_object();
  }

  auto sep_key = ST(sep, isolate);
  auto end_key = ST(end, isolate);
  auto file_key = ST(print_file, isolate);
  auto flush_key = ST(flush, isolate);

  for (int64_t i = 0; i < kwargs->capacity(); ++i) {
    auto item = kwargs->ItemAtIndex(i, isolate);
    if (item.is_null()) {
      continue;
    }
    auto key = item->Get(0, isolate);
    if (!IsPyString(*key)) {
      Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                         "keywords must be strings");
      return kNullMaybeHandle;
    }

    bool accepted = false;
    bool equal = false;
    if (!PyObject::EqualBool(isolate, key, sep_key).To(&equal)) {
      return kNullMaybeHandle;
    }
    accepted |= equal;
    if (!PyObject::EqualBool(isolate, key, end_key).To(&equal)) {
      return kNullMaybeHandle;
    }
    accepted |= equal;
    if (!PyObject::EqualBool(isolate, key, file_key).To(&equal)) {
      return kNullMaybeHandle;
    }
    accepted |= equal;
    if (!PyObject::EqualBool(isolate, key, flush_key).To(&equal)) {
      return kNullMaybeHandle;
    }
    accepted |= equal;
    if (accepted) {
      continue;
    }

    auto key_str = Handle<PyString>::cast(key);
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "'print' got an unexpected keyword argument '%s'",
                        key_str->buffer());
    return kNullMaybeHandle;
  }

  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found,
                                   kwargs->Get(sep_key, options.sep, isolate),
                                   kNullMaybeHandle);
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found,
                                   kwargs->Get(end_key, options.end, isolate),
                                   kNullMaybeHandle);
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, found,
                                   kwargs->Get(file_key, options.file, isolate),
                                   kNullMaybeHandle);
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, found, kwargs->Get(flush_key, options.flush, isolate),
      kNullMaybeHandle);

  if (!options.sep.is_null() && !IsPyNone(options.sep, isolate)) {
    if (!IsPyString(*options.sep)) {
      Runtime_ThrowErrorf(
          isolate, ExceptionType::kTypeError, "sep must be None or str, not %s",
          PyObject::GetTypeName(options.sep, isolate)->buffer());
      return kNullMaybeHandle;
    }
  } else {
    options.sep = Handle<PyObject>::null();
  }

  if (!options.end.is_null() && !IsPyNone(options.end, isolate)) {
    if (!IsPyString(*options.end)) {
      Runtime_ThrowErrorf(
          isolate, ExceptionType::kTypeError, "end must be None or str, not %s",
          PyObject::GetTypeName(options.end, isolate)->buffer());
      return kNullMaybeHandle;
    }
  } else {
    options.end = Handle<PyObject>::null();
  }

  if (!options.file.is_null() && !IsPyNone(options.file, isolate)) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "print() currently only supports file=None");
    return kNullMaybeHandle;
  }

  return isolate->factory()->py_none_object();
}

void WriteRawString(const char* buffer, int64_t length) {
  if (length <= 0) {
    return;
  }
  std::fwrite(buffer, 1, static_cast<size_t>(length), stdout);
}

void WriteHandleString(Handle<PyString> value) {
  WriteRawString(value->buffer(), value->length());
}

}  // namespace

BUILTIN(Print) {
  EscapableHandleScope scope(isolate);

  PrintOptions options;
  RETURN_ON_EXCEPTION_VALUE(isolate,
                            NormalizePrintOptions(isolate, kwargs, options),
                            kNullMaybeHandle);

  const int64_t argc = args.is_null() ? 0 : args->length();
  for (int64_t i = 0; i < argc; ++i) {
    if (i > 0) {
      if (options.sep.is_null()) {
        std::fputc(' ', stdout);
      } else {
        WriteHandleString(Handle<PyString>::cast(options.sep));
      }
    }
    Handle<PyString> value;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, value,
                               PyObject::Str(isolate, args->Get(i, isolate)));
    WriteHandleString(value);
  }

  if (options.end.is_null()) {
    std::fputc('\n', stdout);
  } else {
    WriteHandleString(Handle<PyString>::cast(options.end));
  }

  if (!options.flush.is_null()) {
    if (Runtime_PyObjectIsTrue(isolate, options.flush)) {
      std::fflush(stdout);
    }
  }

  return scope.Escape(isolate->factory()->py_none_object());
}

}  // namespace saauso::internal
