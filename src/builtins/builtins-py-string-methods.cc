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
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
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

BUILTIN_METHOD(PyStringBuiltinMethods, Split) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc > 2) {
    std::fprintf(
        stderr,
        "TypeError: str.split() takes at most 2 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  bool sep_from_positional = false;
  bool maxsplit_from_positional = false;
  Handle<PyObject> sep_obj = Handle<PyObject>::null();
  int64_t maxsplit = -1;

  if (argc >= 1) {
    sep_obj = args->Get(0);
    sep_from_positional = true;
  }
  if (argc == 2) {
    maxsplit = Runtime_DecodeIntLikeOrDie(args->GetTagged(1));
    maxsplit_from_positional = true;
  }

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Handle<PyObject> sep_key = PyString::NewInstance("sep");
    Handle<PyObject> maxsplit_key = PyString::NewInstance("maxsplit");

    for (auto i = 0; i < kwargs->capacity(); ++i) {
      auto item = kwargs->ItemAtIndex(i);
      if (item.is_null()) {
        continue;
      }

      auto key = item->Get(0);
      if (!IsPyString(key)) {
        std::fprintf(stderr, "TypeError: keywords must be strings\n");
        std::exit(1);
      }

      if (PyObject::EqualBool(key, sep_key) ||
          PyObject::EqualBool(key, maxsplit_key)) {
        continue;
      }

      auto key_str = Handle<PyString>::cast(key);
      std::fprintf(stderr,
                   "TypeError: str.split() got an unexpected keyword argument "
                   "'%.*s'\n",
                   static_cast<int>(key_str->length()), key_str->buffer());
      std::exit(1);
    }

    Handle<PyObject> sep_from_kwargs = kwargs->Get(sep_key);
    if (!sep_from_kwargs.is_null()) {
      if (sep_from_positional) {
        std::fprintf(
            stderr,
            "TypeError: str.split() got multiple values for argument 'sep'\n");
        std::exit(1);
      }
      sep_obj = sep_from_kwargs;
    }

    Handle<PyObject> maxsplit_from_kwargs = kwargs->Get(maxsplit_key);
    if (!maxsplit_from_kwargs.is_null()) {
      if (maxsplit_from_positional) {
        std::fprintf(stderr,
                     "TypeError: str.split() got multiple values for argument "
                     "'maxsplit'\n");
        std::exit(1);
      }
      maxsplit = Runtime_DecodeIntLikeOrDie(*maxsplit_from_kwargs);
    }
  }

  Handle<PyObject> sep_or_null = Handle<PyObject>::null();
  if (!sep_obj.is_null() && !IsPyNone(*sep_obj)) {
    if (!IsPyString(*sep_obj)) {
      auto type_name = PyObject::GetKlass(sep_obj)->name();
      std::fprintf(stderr, "TypeError: must be str or None, not %s\n",
                   type_name->buffer());
      std::exit(1);
    }
    sep_or_null = sep_obj;
  }

  Handle<PyList> result = PyString::Split(str_object, sep_or_null, maxsplit);
  return scope.Escape(result);
}

BUILTIN_METHOD(PyStringBuiltinMethods, Join) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str.join() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    std::fprintf(stderr,
                 "TypeError: str.join() takes exactly one argument (%lld "
                 "given)\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  Handle<PyObject> iterable = args->Get(0);
  Handle<PyString> result = PyString::Join(str_object, iterable);
  return scope.Escape(result);
}

}  // namespace saauso::internal
