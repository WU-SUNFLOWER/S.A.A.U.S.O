// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-string-methods.h"

#include <algorithm>
#include <cinttypes>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
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
#include "src/runtime/runtime-conversions.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"
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

namespace {

// 校验 str.index/find/rfind 的公共参数。
// 成功返回 true；失败时抛出 TypeError 并返回 false。
bool ValidateStringSearchArgs(Handle<PyDict> kwargs,
                              Handle<PyTuple> args,
                              const char* method_name,
                              int64_t& argc) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Runtime_ThrowTypeErrorf("%s takes no keyword arguments", method_name);
    return false;
  }

  argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    Runtime_ThrowTypeErrorf("%s takes at least 1 argument (%" PRId64 " given)",
                            method_name, argc);
    return false;
  }
  if (argc > 3) {
    Runtime_ThrowTypeErrorf("%s takes at most 3 arguments (%" PRId64 " given)",
                            method_name, argc);
    return false;
  }

  return true;
}

// 校验搜索目标必须为 str，并解析可选的 begin/end 参数。
// 成功返回 true；失败时抛出异常并返回 false。
bool ParseStringSearchTarget(Handle<PyTuple> args,
                             int64_t argc,
                             int64_t str_length,
                             Handle<PyString>& target_str,
                             int64_t& begin,
                             int64_t& end) {
  auto target = args->Get(0);
  if (!IsPyString(target)) {
    auto type_name = PyObject::GetKlass(target)->name();
    Runtime_ThrowTypeErrorf("must be str, not %.*s",
                            static_cast<int>(type_name->length()),
                            type_name->buffer());
    return false;
  }
  target_str = Handle<PyString>::cast(target);

  begin = 0;
  end = str_length;

  if (argc >= 2) {
    if (!Runtime_DecodeIntLike(args->GetTagged(1)).To(&begin)) {
      return false;
    }
  }
  if (argc >= 3) {
    if (!Runtime_DecodeIntLike(args->GetTagged(2)).To(&end)) {
      return false;
    }
  }

  // 负索引折叠与裁剪
  if (begin < 0) {
    begin += str_length;
  }
  if (end < 0) {
    end += str_length;
  }
  begin = std::min(std::max(static_cast<int64_t>(0), begin), str_length);
  end = std::min(std::max(static_cast<int64_t>(0), end), str_length);

  return true;
}

}  // namespace

BUILTIN_METHOD(PyStringBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  int64_t argc = 0;
  if (!ValidateStringSearchArgs(kwargs, args, "str.index()", argc)) {
    return kNullMaybeHandle;
  }

  Handle<PyString> target_str;
  int64_t begin = 0;
  int64_t end = 0;
  if (!ParseStringSearchTarget(args, argc, str_object->length(), target_str,
                               begin, end)) {
    return kNullMaybeHandle;
  }

  int64_t result = PyString::kNotFound;
  if (begin <= end) {
    result = str_object->IndexOf(target_str, begin, end);
  }
  if (result == PyString::kNotFound) {
    Runtime_ThrowValueError("substring not found");
    return kNullMaybeHandle;
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

BUILTIN_METHOD(PyStringBuiltinMethods, Find) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  int64_t argc = 0;
  if (!ValidateStringSearchArgs(kwargs, args, "str.find()", argc)) {
    return kNullMaybeHandle;
  }

  Handle<PyString> target_str;
  int64_t begin = 0;
  int64_t end = 0;
  if (!ParseStringSearchTarget(args, argc, str_object->length(), target_str,
                               begin, end)) {
    return kNullMaybeHandle;
  }

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

  int64_t argc = 0;
  if (!ValidateStringSearchArgs(kwargs, args, "str.rfind()", argc)) {
    return kNullMaybeHandle;
  }

  Handle<PyString> target_str;
  int64_t begin = 0;
  int64_t end = 0;
  if (!ParseStringSearchTarget(args, argc, str_object->length(), target_str,
                               begin, end)) {
    return kNullMaybeHandle;
  }

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
    Runtime_ThrowTypeErrorf(
        "str.split() takes at most 2 arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }

  bool sep_from_positional = false;
  bool maxsplit_from_positional = false;
  Handle<PyObject> sep_obj = Handle<PyObject>::null();
  int64_t maxsplit = -1;

  auto* isolate = Isolate::Current();

  if (argc >= 1) {
    sep_obj = args->Get(0);
    sep_from_positional = true;
  }
  if (argc == 2) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, maxsplit,
                               Runtime_DecodeIntLike(args->GetTagged(1)));
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
        Runtime_ThrowTypeError("keywords must be strings");
        return kNullMaybeHandle;
      }

      if (PyObject::EqualBool(key, sep_key) ||
          PyObject::EqualBool(key, maxsplit_key)) {
        continue;
      }

      auto key_str = Handle<PyString>::cast(key);
      Runtime_ThrowTypeErrorf(
          "str.split() got an unexpected keyword argument '%s'",
          key_str->buffer());
      return kNullMaybeHandle;
    }

    Handle<PyObject> sep_from_kwargs = kwargs->Get(sep_key);
    if (!sep_from_kwargs.is_null()) {
      if (sep_from_positional) {
        Runtime_ThrowTypeError(
            "str.split() got multiple values for argument 'sep'");
        return kNullMaybeHandle;
      }
      sep_obj = sep_from_kwargs;
    }

    Handle<PyObject> maxsplit_from_kwargs = kwargs->Get(maxsplit_key);
    if (!maxsplit_from_kwargs.is_null()) {
      if (maxsplit_from_positional) {
        Runtime_ThrowTypeError(
            "str.split() got multiple values for argument 'maxsplit'");
        return kNullMaybeHandle;
      }

      ASSIGN_RETURN_ON_EXCEPTION(isolate, maxsplit,
                                 Runtime_DecodeIntLike(*maxsplit_from_kwargs));
    }
  }

  Handle<PyObject> sep_or_null = Handle<PyObject>::null();
  if (!sep_obj.is_null() && !IsPyNone(*sep_obj)) {
    if (!IsPyString(*sep_obj)) {
      auto type_name = PyObject::GetKlass(sep_obj)->name();
      Runtime_ThrowTypeErrorf("must be str or None, not %.*s",
                              static_cast<int>(type_name->length()),
                              type_name->buffer());
      return kNullMaybeHandle;
    }
    sep_or_null = sep_obj;
  }

  Handle<PyList> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Runtime_PyStringSplit(str_object, sep_or_null, maxsplit));

  return scope.Escape(result);
}

BUILTIN_METHOD(PyStringBuiltinMethods, Join) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Runtime_ThrowTypeError("str.join() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowTypeErrorf(
        "str.join() takes exactly one argument (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }

  Handle<PyObject> iterable = args->Get(0);
  Handle<PyString> result;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), result,
                             Runtime_PyStringJoin(str_object, iterable));

  return scope.Escape(result);
}

}  // namespace saauso::internal
