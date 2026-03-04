// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-dict.h"

#include <cinttypes>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Maybe<bool> Runtime_InitDictFromArgsKwargs(Handle<PyDict> result,
                                           Handle<PyObject> args,
                                           Handle<PyObject> kwargs) {
  auto* isolate [[maybe_unused]] = Isolate::Current();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc > 1) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "dict expected at most 1 argument, got %" PRId64, argc);
    return kNullMaybe;
  }

  if (argc == 1) {
    Handle<PyObject> input = pos_args->Get(0);
    if (IsPyDict(input)) {
      auto src = Handle<PyDict>::cast(input);
      for (int64_t i = 0; i < src->capacity(); ++i) {
        Handle<PyTuple> item = src->ItemAtIndex(i);
        if (item.is_null()) {
          continue;
        }
        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, item->Get(0), item->Get(1)),
            kNullMaybe);
      }
    } else {
      Handle<PyTuple> elements;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, elements,
                                 Runtime_UnpackIterableObjectToTuple(input));
      for (int64_t i = 0; i < elements->length(); ++i) {
        Handle<PyObject> elem = elements->Get(i);
        Handle<PyTuple> pair;
        ASSIGN_RETURN_ON_EXCEPTION(isolate, pair,
                                   Runtime_UnpackIterableObjectToTuple(elem));
        if (pair->length() != 2) {
          Runtime_ThrowErrorf(ExceptionType::kTypeError,
                              "cannot convert dictionary update sequence "
                              "element #%" PRId64 " to a sequence",
                              i);
          return kNullMaybe;
        }

        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, pair->Get(0), pair->Get(1)),
            kNullMaybe);
      }
    }
  }

  if (!kwargs.is_null()) {
    Handle<PyDict> kw = Handle<PyDict>::cast(kwargs);
    if (!kw.is_null() && kw->occupied() != 0) {
      for (int64_t i = 0; i < kw->capacity(); ++i) {
        Handle<PyTuple> item = kw->ItemAtIndex(i);
        if (item.is_null()) {
          continue;
        }

        Handle<PyObject> key = item->Get(0);
        if (!IsPyString(key)) {
          Handle<PyString> key_str;
          if (!Runtime_NewStr(key).ToHandle(&key_str)) {
            return kNullMaybe;
          }
          key = key_str;
        }

        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, key, item->Get(1)), kNullMaybe);
      }
    }
  }

  return Maybe<bool>(true);
}

MaybeHandle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                      Handle<PyObject> kwargs) {
  EscapableHandleScope scope;
  auto* isolate [[maybe_unused]] = Isolate::Current();
  Handle<PyDict> result = PyDict::NewInstance();
  bool ok = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, ok, Runtime_InitDictFromArgsKwargs(result, args, kwargs));
  if (!ok) {
    return kNullMaybeHandle;
  }
  return scope.Escape(result);
}

MaybeHandle<PyObject> Runtime_DictGetItem(Handle<PyDict> dict,
                                          Handle<PyObject> key) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value).To(&found)) {
    return kNullMaybeHandle;
  }
  if (!found) {
    Runtime_ThrowError(ExceptionType::kKeyError, "key not found in dict");
    return kNullMaybeHandle;
  }
  assert(!value.is_null());
  return value;
}

MaybeHandle<PyObject> Runtime_DictSetItem(Handle<PyDict> dict,
                                          Handle<PyObject> key,
                                          Handle<PyObject> value) {
  if (PyDict::Put(dict, key, value).IsEmpty()) {
    return kNullMaybeHandle;
  }
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Runtime_DictDelItem(Handle<PyDict> dict,
                                          Handle<PyObject> key) {
  bool removed = false;
  if (!dict->Remove(key).To(&removed)) {
    return kNullMaybeHandle;
  }
  if (!removed) {
    Runtime_ThrowError(ExceptionType::kKeyError, "key not found in dict");
    return kNullMaybeHandle;
  }
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Runtime_DictGet(Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!value.is_null());
    return value;
  }
  if (!default_or_null.is_null()) {
    return default_or_null;
  }
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> Runtime_DictSetDefault(Handle<PyDict> dict,
                                             Handle<PyObject> key,
                                             Handle<PyObject> default_or_null) {
  Handle<PyObject> existing;
  bool found = false;
  if (!dict->Get(key, existing).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!existing.is_null());
    return existing;
  }

  Handle<PyObject> value = default_or_null.is_null()
                               ? handle(Isolate::Current()->py_none_object())
                               : default_or_null;

  if (PyDict::Put(dict, key, value).IsEmpty()) {
    return kNullMaybeHandle;
  }
  return value;
}

MaybeHandle<PyObject> Runtime_DictPop(Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null,
                                      bool has_default) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!value.is_null());
    bool removed = false;
    if (!dict->Remove(key).To(&removed)) {
      return kNullMaybeHandle;
    }
    if (!removed) {
      Runtime_ThrowError(ExceptionType::kKeyError, nullptr);
      return kNullMaybeHandle;
    }
    return value;
  }

  if (has_default) {
    return default_or_null;
  }

  Runtime_ThrowError(ExceptionType::kKeyError, nullptr);
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> Runtime_MergeDict(Isolate* isolate,
                                        Handle<PyDict> dst_dict,
                                        Handle<PyDict> source_dict,
                                        bool allow_overwriting) {
  EscapableHandleScope scope;

  auto iter = isolate->factory()->NewPyDictItemIterator(source_dict);
  while (true) {
    Handle<PyObject> item_handle;
    if (!PyObject::Next(iter).ToHandle(&item_handle)) {
      bool is_stop_iteration = false;
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, is_stop_iteration,
          Runtime_ConsumePendingStopIterationIfSet(isolate));
      if (!is_stop_iteration) {
        return kNullMaybeHandle;
      }
      break;
    }

    auto item = Handle<PyTuple>::cast(item_handle);
    auto key = item->Get(0);
    auto value = item->Get(1);

    bool exists = false;
    ASSIGN_RETURN_ON_EXCEPTION_VALUE(
        isolate, exists, dst_dict->ContainsKey(key), kNullMaybeHandle);
    if (!allow_overwriting && exists) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "got multiple values for keyword argument");
      return kNullMaybeHandle;
    }

    RETURN_ON_EXCEPTION_VALUE(isolate, PyDict::Put(dst_dict, key, value),
                              kNullMaybeHandle);
  }

  return scope.Escape(dst_dict);
}

}  // namespace saauso::internal
