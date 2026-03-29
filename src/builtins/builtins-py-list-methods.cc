// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-list-methods.h"

#include <algorithm>
#include <cinttypes>
#include <vector>

#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-conversions.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-py-tuple.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/utils/stable-merge-sort.h"

namespace saauso::internal {

Maybe<void> PyListBuiltinMethods::Install(Isolate* isolate,
                                          Handle<PyDict> target,
                                          Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_LIST_BUILTINS(INSTALL_BUILTIN_METHOD);

#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyListBuiltinMethods, New) {
  Handle<PyObject> type_object;
  Handle<PyObject> new_args = args;

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc == 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "descriptor '__new__' of 'list' object needs an "
                       "argument");
    return kNullMaybeHandle;
  }
  type_object = args->Get(0);
  new_args = Runtime_NewTupleTailOrNull(isolate, args, 1);

  if (!IsPyTypeObject(type_object)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "list.__new__() argument 1 must be type, not '%s'",
                        PyObject::GetTypeName(type_object, isolate)->buffer());
    return kNullMaybeHandle;
  }

  return PyListKlass::GetInstance(isolate)->NewInstance(
      isolate, Handle<PyTypeObject>::cast(type_object), new_args, kwargs);
}

BUILTIN_METHOD(PyListBuiltinMethods, Append) {
  EscapableHandleScope scope;

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 1) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "list.append() takes exactly one argument (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }

  auto object = Handle<PyList>::cast(self);
  PyList::Append(object, args->Get(0), isolate);
  return scope.Escape(handle(isolate->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Init) {
  return PyListKlass::GetInstance(isolate)->InitInstance(isolate, self, args,
                                                         kwargs);
}

BUILTIN_METHOD(PyListBuiltinMethods, Repr) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "list.__repr__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Repr(isolate, self);
}

BUILTIN_METHOD(PyListBuiltinMethods, Str) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "list.__str__() takes no arguments (%" PRId64 " given)",
                        argc);
    return kNullMaybeHandle;
  }
  return PyObject::Str(isolate, self);
}

BUILTIN_METHOD(PyListBuiltinMethods, Pop) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  if (object->IsEmpty()) {
    Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                       "pop from empty list");
    return kNullMaybeHandle;
  }
  return scope.Escape(object->Pop());
}

BUILTIN_METHOD(PyListBuiltinMethods, Insert) {
  EscapableHandleScope scope;
  auto object = Handle<PyList>::cast(self);
  auto index = Handle<PySmi>::cast(args->Get(0));
  PyList::Insert(object, PySmi::ToInt(index), args->Get(1), isolate);
  return scope.Escape(handle(isolate->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "list.index() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "list.index() takes at least 1 argument (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  if (argc > 3) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "list.index() takes at most 3 arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }

  auto target = args->Get(0);

  int64_t length = list->length();
  int64_t begin = 0;
  int64_t end = length;

  if (argc >= 2) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, begin,
                               Runtime_DecodeIntLike(isolate, *args->Get(1)));
  }
  if (argc >= 3) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, end,
                               Runtime_DecodeIntLike(isolate, *args->Get(2)));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyList::kNotFound;
  if (begin <= end) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, result,
                               list->IndexOf(target, begin, end, isolate));
  }
  if (result == PyList::kNotFound) {
    Handle<PyString> repr_content;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, repr_content,
                               PyObject::Repr(isolate, target));
    Runtime_ThrowErrorf(isolate, ExceptionType::kValueError,
                        "%s is not in list", repr_content->buffer());
    return kNullMaybeHandle;
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

BUILTIN_METHOD(PyListBuiltinMethods, Reverse) {
  EscapableHandleScope scope;
  auto list = Handle<PyList>::cast(self);

  auto length = list->length();
  for (auto i = 0; i < (length >> 1); ++i) {
    Handle<PyObject> tmp = list->Get(i);
    list->Set(i, list->Get(length - i - 1));
    list->Set(length - i - 1, tmp);
  }

  return scope.Escape(handle(isolate->py_none_object()));
}

BUILTIN_METHOD(PyListBuiltinMethods, Extend) {
  if (Runtime_ExtendListByItratableObject(isolate, Handle<PyList>::cast(self),
                                          args->Get(0))
          .IsEmpty()) {
    return kNullMaybeHandle;
  }
  return handle(isolate->py_none_object());
}

BUILTIN_METHOD(PyListBuiltinMethods, Sort) {
  EscapableHandleScope scope;

  if (!args.is_null() && args->length() != 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "sort() takes no positional arguments");
    return kNullMaybeHandle;
  }

  auto list = Handle<PyList>::cast(self);
  int64_t expected_length = list->length();
  if (expected_length <= 1) {
    return scope.Escape(handle(isolate->py_none_object()));
  }

  Handle<PyObject> key_func;
  bool reverse = false;

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Handle<PyString> key_name = PyString::New(isolate, "key");
    Handle<PyString> reverse_name = PyString::New(isolate, "reverse");

    for (int64_t i = 0; i < kwargs->capacity(); ++i) {
      Handle<PyObject> k = kwargs->KeyAtIndex(i);
      if (k.is_null()) {
        continue;
      }
      if (!IsPyString(*k)) {
        Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                           "sort() keywords must be strings");
        return kNullMaybeHandle;
      }
      auto key_str = Handle<PyString>::cast(k);
      if (!key_str->IsEqualTo(*key_name) &&
          !key_str->IsEqualTo(*reverse_name)) {
        Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                            "sort() got an unexpected keyword argument");
        return kNullMaybeHandle;
      }
    }

    // 查找关键字提取函数
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                               kwargs->Get(key_name, key_func, isolate));
    assert(!found || !key_func.is_null());

    // 查找倒序排序控制参数
    Handle<PyObject> reverse_flag;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, found, kwargs->Get(reverse_name, reverse_flag, isolate));
    if (found) {
      assert(!reverse_flag.is_null());
      reverse = Runtime_PyObjectIsTrue(reverse_flag);
    }
  }

  if (!key_func.is_null() && !IsNormalPyFunction(key_func, isolate) &&
      !IsNativePyFunction(key_func, isolate) && !IsMethodObject(key_func)) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "key must be callable");
    return kNullMaybeHandle;
  }

  Handle<FixedArray> keys = isolate->factory()->NewFixedArray(expected_length);

  if (key_func.is_null()) {
    for (int64_t i = 0; i < expected_length; ++i) {
      keys->Set(i, list->Get(i));
    }
  } else {
    Handle<PyTuple> key_args = PyTuple::New(isolate, 1);
    Handle<PyDict> empty_kwargs = PyDict::New(isolate);

    for (int64_t i = 0; i < expected_length; ++i) {
      if (list->length() != expected_length) {
        Runtime_ThrowError(isolate, ExceptionType::kValueError,
                           "list modified during sort (key)");
        return kNullMaybeHandle;
      }
      Handle<PyObject> elem = list->Get(i);
      key_args->SetInternal(0, elem);
      Handle<PyObject> key;
      if (!Execution::Call(isolate, key_func, Handle<PyObject>::null(),
                           key_args, empty_kwargs)
               .ToHandle(&key)) {
        return kNullMaybeHandle;
      }
      keys->Set(i, key);
    }
  }

  std::vector<int64_t> indices(static_cast<size_t>(expected_length));
  for (int64_t i = 0; i < expected_length; ++i) {
    indices[static_cast<size_t>(i)] = i;
  }

  struct CompareContext {
    Isolate* isolate;
    Handle<PyList> list;
    Handle<FixedArray> keys;
    int64_t expected_length;
  };

  CompareContext context{isolate, list, keys, expected_length};

  // 排序比较回调。若排序期间检测到 list 长度被修改，则抛出 ValueError 并
  // 返回 false。排序完成后由外层检查 pending exception 进行 unwind。
  auto less = [](int64_t a, int64_t b, void* ctx) -> bool {
    auto* c = static_cast<CompareContext*>(ctx);
    if (c->list->length() != c->expected_length) {
      Runtime_ThrowError(c->isolate, ExceptionType::kValueError,
                         "list modified during sort");
      return false;
    }
    HandleScope scope;
    Handle<PyObject> ka = handle(c->keys->Get(a));
    Handle<PyObject> kb = handle(c->keys->Get(b));
    Maybe<bool> mb = PyObject::LessBool(c->isolate, ka, kb);
    if (mb.IsNothing()) {
      return false;
    }
    return mb.ToChecked();
  };

  StableMergeSort::Sort(indices.data(), expected_length, less, &context);

  // 排序回调中可能已设置 pending exception（list 被修改等），此时排序结果
  // 无意义，直接传播异常。
  if (isolate->HasPendingException()) {
    return kNullMaybeHandle;
  }

  Handle<FixedArray> tmp = isolate->factory()->NewFixedArray(expected_length);
  for (int64_t i = 0; i < expected_length; ++i) {
    tmp->Set(i, list->Get(indices[static_cast<size_t>(i)]));
  }
  for (int64_t i = 0; i < expected_length; ++i) {
    list->Set(i, handle(tmp->Get(i)));
  }

  if (reverse) {
    for (int64_t i = 0; i < (expected_length >> 1); ++i) {
      Handle<PyObject> t = list->Get(i);
      list->Set(i, list->Get(expected_length - i - 1));
      list->Set(expected_length - i - 1, t);
    }
  }

  return scope.Escape(handle(isolate->py_none_object()));
}

}  // namespace saauso::internal
