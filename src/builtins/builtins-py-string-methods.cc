// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-string-methods.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "src/handles/handles.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime.h"
#include "src/utils/utils.h"

namespace saauso::internal {

void PyStringBuiltinMethods::Install(Handle<PyDict> target) {
  PY_STRING_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyStringBuiltinMethods, Upper) {
  EscapableHandleScope scope;

  auto str_object = Handle<PyString>::cast(self);
  auto result =
      PyString::NewInstance(str_object->buffer(), str_object->length());

  for (auto i = 0; i < result->length(); ++i) {
    char ch = result->Get(i);
    if (InRangeWithRightClose(ch, 'a', 'z')) {
      result->Set(i, ch - 'a' + 'A');
    }
  }

  return scope.Escape(result);
}

BUILTIN_METHOD(PyStringBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str.index() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: str.index() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: str.index() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);
  if (!IsPyString(target)) {
    auto type_name = PyObject::GetKlass(target)->name();
    std::fprintf(stderr, "TypeError: must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }

  auto target_str = Handle<PyString>::cast(target);

  int64_t length = str_object->length();
  int64_t begin = 0;
  int64_t end = length;
  if (argc >= 2) {
    begin = Runtime_DecodeIntLikeOrDie(args->GetTagged(1));
  }
  if (argc >= 3) {
    end = Runtime_DecodeIntLikeOrDie(args->GetTagged(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyString::kNotFound;
  if (begin <= end) {
    result = str_object->IndexOf(target_str, begin, end);
  }
  if (result == PyString::kNotFound) {
    std::fprintf(stderr, "ValueError: substring not found\n");
    std::exit(1);
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

BUILTIN_METHOD(PyStringBuiltinMethods, Find) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str.find() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: str.find() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: str.find() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);
  if (!IsPyString(target)) {
    auto type_name = PyObject::GetKlass(target)->name();
    std::fprintf(stderr, "TypeError: must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }
  auto target_str = Handle<PyString>::cast(target);

  int64_t length = str_object->length();
  int64_t begin = 0;
  int64_t end = length;
  if (argc >= 2) {
    begin = Runtime_DecodeIntLikeOrDie(args->GetTagged(1));
  }
  if (argc >= 3) {
    end = Runtime_DecodeIntLikeOrDie(args->GetTagged(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyString::kNotFound;
  if (begin <= end) {
    result = str_object->IndexOf(target_str, begin, end);
  }
  if (result == PyString::kNotFound) {
    result = -1;
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

BUILTIN_METHOD(PyStringBuiltinMethods, Rfind) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str.rfind() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: str.rfind() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: str.rfind() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);
  if (!IsPyString(target)) {
    auto type_name = PyObject::GetKlass(target)->name();
    std::fprintf(stderr, "TypeError: must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }
  auto target_str = Handle<PyString>::cast(target);

  int64_t length = str_object->length();
  int64_t begin = 0;
  int64_t end = length;
  if (argc >= 2) {
    begin = Runtime_DecodeIntLikeOrDie(args->GetTagged(1));
  }
  if (argc >= 3) {
    end = Runtime_DecodeIntLikeOrDie(args->GetTagged(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyString::kNotFound;
  if (begin <= end) {
    result = str_object->LastIndexOf(target_str, begin, end);
  }
  if (result == PyString::kNotFound) {
    result = -1;
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

}  // namespace saauso::internal
