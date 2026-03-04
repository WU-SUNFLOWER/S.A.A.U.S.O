// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-string.h"

#include <cassert>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/string-search.h"

namespace saauso::internal {

MaybeHandle<PyList> Runtime_PyStringSplit(Handle<PyString> str,
                                          Handle<PyObject> sep_or_null,
                                          int64_t maxsplit) {
  EscapableHandleScope scope;

  Handle<PyList> result = PyList::NewInstance();

  const int64_t str_length = str->length();
  if (maxsplit == 0) {
    if (sep_or_null.is_null()) {
      int64_t i = 0;
      while (i < str_length &&
             std::isspace(static_cast<unsigned char>(str->Get(i)))) {
        ++i;
      }
      if (i == str_length) {
        return scope.Escape(result);
      }
      int64_t end = str_length;
      while (end > i &&
             std::isspace(static_cast<unsigned char>(str->Get(end - 1)))) {
        --end;
      }
      Handle<PyString> part = PyString::NewInstance(end - i);
      if (end > i) {
        std::memcpy(part->writable_buffer(), str->buffer() + i, end - i);
      }
      PyList::Append(result, part);
      return scope.Escape(result);
    }

    Handle<PyString> part = PyString::NewInstance(str_length);
    if (str_length > 0) {
      std::memcpy(part->writable_buffer(), str->buffer(), str_length);
    }
    PyList::Append(result, part);
    return scope.Escape(result);
  }

  auto make_substring = [&](int64_t begin, int64_t end) -> Handle<PyString> {
    assert(0 <= begin && begin <= end && end <= str_length);
    const int64_t part_length = end - begin;
    Handle<PyString> part = PyString::NewInstance(part_length);
    if (part_length > 0) {
      std::memcpy(part->writable_buffer(), str->buffer() + begin, part_length);
    }
    return part;
  };

  const int64_t max_splits =
      maxsplit < 0 ? std::numeric_limits<int64_t>::max() : maxsplit;

  if (sep_or_null.is_null()) {
    int64_t i = 0;
    while (i < str_length &&
           std::isspace(static_cast<unsigned char>(str->Get(i)))) {
      ++i;
    }
    if (i == str_length) {
      return scope.Escape(result);
    }

    int64_t splits_left = max_splits;
    while (i < str_length) {
      const int64_t start = i;
      while (i < str_length &&
             !std::isspace(static_cast<unsigned char>(str->Get(i)))) {
        ++i;
      }
      PyList::Append(result, make_substring(start, i));

      while (i < str_length &&
             std::isspace(static_cast<unsigned char>(str->Get(i)))) {
        ++i;
      }

      if (i >= str_length) {
        break;
      }

      --splits_left;
      if (splits_left == 0) {
        int64_t end = str_length;
        while (end > i &&
               std::isspace(static_cast<unsigned char>(str->Get(end - 1)))) {
          --end;
        }
        if (i < end) {
          PyList::Append(result, make_substring(i, end));
        }
        break;
      }
    }
    return scope.Escape(result);
  }

  Handle<PyString> sep = Handle<PyString>::cast(sep_or_null);
  if (sep->length() == 0) {
    Runtime_ThrowError(ExceptionType::kValueError, "empty separator");
    return kNullMaybeHandle;
  }

  int64_t splits_left = max_splits;
  int64_t pos = 0;
  while (pos <= str_length && splits_left > 0) {
    const int64_t offset = StringSearch::IndexOfSubstring(
        std::string_view(str->buffer() + pos, str_length - pos),
        std::string_view(sep->buffer(), sep->length()));
    if (offset == StringSearch::kNotFound) {
      break;
    }

    const int64_t found = pos + offset;
    PyList::Append(result, make_substring(pos, found));

    pos = found + sep->length();
    --splits_left;
  }
  PyList::Append(result, make_substring(pos, str_length));
  return scope.Escape(result);
}

MaybeHandle<PyString> Runtime_PyStringJoin(Handle<PyString> str,
                                           Handle<PyObject> iterable) {
  EscapableHandleScope scope;

  if (iterable.is_null()) {
    Runtime_ThrowError(ExceptionType::kTypeError, "can only join an iterable");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> parts;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), parts,
                             Runtime_UnpackIterableObjectToTuple(iterable));

  const int64_t num_parts = parts->length();
  if (num_parts == 0) {
    return scope.Escape(PyString::NewInstance(static_cast<int64_t>(0)));
  }

  const int64_t sep_length = str->length();
  int64_t total_length = 0;

  for (int64_t i = 0; i < num_parts; ++i) {
    Handle<PyObject> item = parts->Get(i);
    if (!PyString::IsStringLike(*item)) {
      auto type_name = PyObject::GetKlass(item)->name();
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "sequence item %" PRId64
                          ": expected str instance, %s found",
                          i, type_name->buffer());
      return kNullMaybeHandle;
    }

    int64_t item_length = Handle<PyString>::cast(item)->length();
    if (item_length > std::numeric_limits<int64_t>::max() - total_length) {
      Runtime_ThrowError(ExceptionType::kValueError,
                         "join() result is too long");
      return kNullMaybeHandle;
    }
    total_length += item_length;

    if (i + 1 < num_parts) {
      if (sep_length > std::numeric_limits<int64_t>::max() - total_length) {
        Runtime_ThrowError(ExceptionType::kValueError,
                           "join() result is too long");
        return kNullMaybeHandle;
      }
      total_length += sep_length;
    }
  }

  Handle<PyString> result = PyString::NewInstance(total_length);
  char* dst = result->writable_buffer();

  int64_t dst_offset = 0;
  for (int64_t i = 0; i < num_parts; ++i) {
    Handle<PyString> part = Handle<PyString>::cast(parts->Get(i));
    if (part->length() > 0) {
      std::memcpy(dst + dst_offset, part->buffer(), part->length());
      dst_offset += part->length();
    }
    if (i + 1 < num_parts && sep_length > 0) {
      std::memcpy(dst + dst_offset, str->buffer(), sep_length);
      dst_offset += sep_length;
    }
  }

  return scope.Escape(result);
}

MaybeHandle<PyString> Runtime_NewStr(Handle<PyObject> value) {
  EscapableHandleScope scope;
  auto* isolate = Isolate::Current();

  if (value.is_null()) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "str() argument must not be null");
    return kNullMaybeHandle;
  }

  if (PyString::IsStringLike(value)) {
    Handle<PyString> s = Handle<PyString>::cast(value);
    if (IsPyString(value)) {
      return scope.Escape(s);
    }
    return scope.Escape(PyString::NewInstance(s->buffer(), s->length()));
  }
  if (IsPySmi(value)) {
    return scope.Escape(PyString::FromPySmi(Tagged<PySmi>::cast(*value)));
  }
  if (IsPyFloat(value)) {
    return scope.Escape(PyString::FromPyFloat(Handle<PyFloat>::cast(value)));
  }
  if (IsPyBoolean(value)) {
    bool v = Tagged<PyBoolean>::cast(*value)->value();
    return scope.Escape(PyString::NewInstance(v ? "True" : "False"));
  }
  if (IsPyNone(value)) {
    return scope.Escape(PyString::NewInstance("None"));
  }

  Handle<PyObject> method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, value, ST(str), method));

  if (!method.is_null()) {
    Handle<PyObject> result;

    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, result,
        Execution::Call(isolate, method, value, Handle<PyTuple>::null(),
                        Handle<PyDict>::null()));

    if (!IsPyStringExact(result)) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "__str__ returned non-string");
      return kNullMaybeHandle;
    }

    return scope.Escape(Handle<PyString>::cast(result));
  }

  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "<object at %p>",
                reinterpret_cast<void*>((*value).ptr()));
  return scope.Escape(PyString::NewInstance(buffer));
}

}  // namespace saauso::internal
