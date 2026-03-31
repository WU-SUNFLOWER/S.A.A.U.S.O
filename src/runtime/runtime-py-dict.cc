// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-dict.h"

#include <cinttypes>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Maybe<void> Runtime_InitDictFromArgsKwargs(Isolate* isolate,
                                           Handle<PyDict> result,
                                           Handle<PyObject> args,
                                           Handle<PyObject> kwargs) {
  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc > 1) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "dict expected at most 1 argument, got %" PRId64, argc);
    return kNullMaybe;
  }

  if (argc == 1) {
    Handle<PyObject> input = pos_args->Get(0);
    if (IsPyDict(input)) {
      auto src = Handle<PyDict>::cast(input);
      for (int64_t i = 0; i < src->capacity(); ++i) {
        Handle<PyTuple> item = src->ItemAtIndex(i, isolate);
        if (item.is_null()) {
          continue;
        }
        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, item->Get(0), item->Get(1), isolate),
            kNullMaybe);
      }
    } else {
      Handle<PyTuple> elements;
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, elements,
          Runtime_UnpackIterableObjectToTuple(isolate, input));
      for (int64_t i = 0; i < elements->length(); ++i) {
        Handle<PyObject> elem = elements->Get(i);
        Handle<PyTuple> pair;
        ASSIGN_RETURN_ON_EXCEPTION(
            isolate, pair, Runtime_UnpackIterableObjectToTuple(isolate, elem));
        if (pair->length() != 2) {
          Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                              "cannot convert dictionary update sequence "
                              "element #%" PRId64 " to a sequence",
                              i);
          return kNullMaybe;
        }

        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, pair->Get(0), pair->Get(1), isolate),
            kNullMaybe);
      }
    }
  }

  if (!kwargs.is_null()) {
    Handle<PyDict> kw = Handle<PyDict>::cast(kwargs);
    if (!kw.is_null() && kw->occupied() != 0) {
      for (int64_t i = 0; i < kw->capacity(); ++i) {
        Handle<PyTuple> item = kw->ItemAtIndex(i, isolate);
        if (item.is_null()) {
          continue;
        }

        Handle<PyObject> key = item->Get(0);
        if (!IsPyString(key)) {
          Handle<PyString> key_str;
          ASSIGN_RETURN_ON_EXCEPTION(isolate, key_str,
                                     PyObject::Str(isolate, key));
          key = key_str;
        }

        RETURN_ON_EXCEPTION_VALUE(
            isolate, PyDict::Put(result, key, item->Get(1), isolate),
            kNullMaybe);
      }
    }
  }

  return JustVoid();
}

MaybeHandle<PyObject> Runtime_DictGetItem(Isolate* isolate,
                                          Handle<PyDict> dict,
                                          Handle<PyObject> key) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value, isolate).To(&found)) {
    return kNullMaybeHandle;
  }
  if (!found) {
    Runtime_ThrowError(isolate, ExceptionType::kKeyError,
                       "key not found in dict");
    return kNullMaybeHandle;
  }
  assert(!value.is_null());
  return value;
}

MaybeHandle<PyObject> Runtime_DictSetItem(Isolate* isolate,
                                          Handle<PyDict> dict,
                                          Handle<PyObject> key,
                                          Handle<PyObject> value) {
  if (PyDict::Put(dict, key, value, isolate).IsEmpty()) {
    return kNullMaybeHandle;
  }
  return isolate->factory()->py_none_object();
}

MaybeHandle<PyObject> Runtime_DictDelItem(Isolate* isolate,
                                          Handle<PyDict> dict,
                                          Handle<PyObject> key) {
  bool removed = false;
  if (!dict->Remove(key, isolate).To(&removed)) {
    return kNullMaybeHandle;
  }
  if (!removed) {
    Runtime_ThrowError(isolate, ExceptionType::kKeyError,
                       "key not found in dict");
    return kNullMaybeHandle;
  }
  return isolate->factory()->py_none_object();
}

MaybeHandle<PyObject> Runtime_DictGet(Isolate* isolate,
                                      Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value, isolate).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!value.is_null());
    return value;
  }
  if (!default_or_null.is_null()) {
    return default_or_null;
  }
  return isolate->factory()->py_none_object();
}

MaybeHandle<PyObject> Runtime_DictSetDefault(Isolate* isolate,
                                             Handle<PyDict> dict,
                                             Handle<PyObject> key,
                                             Handle<PyObject> default_or_null) {
  Handle<PyObject> existing;
  bool found = false;
  if (!dict->Get(key, existing, isolate).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!existing.is_null());
    return existing;
  }

  Handle<PyObject> value = default_or_null.is_null()
                               ? isolate->factory()->py_none_object()
                               : default_or_null;

  if (PyDict::Put(dict, key, value, isolate).IsEmpty()) {
    return kNullMaybeHandle;
  }
  return value;
}

MaybeHandle<PyObject> Runtime_DictPop(Isolate* isolate,
                                      Handle<PyDict> dict,
                                      Handle<PyObject> key,
                                      Handle<PyObject> default_or_null,
                                      bool has_default) {
  Handle<PyObject> value;
  bool found = false;
  if (!dict->Get(key, value, isolate).To(&found)) {
    return kNullMaybeHandle;
  }
  if (found) {
    assert(!value.is_null());
    bool removed = false;
    if (!dict->Remove(key, isolate).To(&removed)) {
      return kNullMaybeHandle;
    }
    if (!removed) {
      Runtime_ThrowError(isolate, ExceptionType::kKeyError, nullptr);
      return kNullMaybeHandle;
    }
    return value;
  }

  if (has_default) {
    return default_or_null;
  }

  Runtime_ThrowError(isolate, ExceptionType::kKeyError, nullptr);
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
    if (!PyObject::Next(isolate, iter).ToHandle(&item_handle)) {
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
        isolate, exists, dst_dict->ContainsKey(key, isolate), kNullMaybeHandle);
    if (!allow_overwriting && exists) {
      Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                         "got multiple values for keyword argument");
      return kNullMaybeHandle;
    }

    RETURN_ON_EXCEPTION_VALUE(
        isolate, PyDict::Put(dst_dict, key, value, isolate), kNullMaybeHandle);
  }

  return scope.Escape(dst_dict);
}

MaybeHandle<PyString> Runtime_NewDictRepr(Isolate* isolate,
                                          Handle<PyDict> dict) {
  EscapableHandleScope scope;

  std::string repr("{");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Tagged<PyObject> key_tagged = dict->data()->Get(i << 1);
    if (key_tagged.is_null()) {
      continue;
    }
    if (!first) {
      repr.append(", ");
    }
    first = false;
    Handle<PyString> key_repr;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, key_repr,
        PyObject::Repr(isolate, handle(key_tagged, isolate)));
    repr.append(key_repr->ToStdString());
    repr.append(": ");
    Handle<PyString> value_repr;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, value_repr,
        PyObject::Repr(isolate,
                       handle(dict->data()->Get((i << 1) + 1), isolate)));
    repr.append(value_repr->ToStdString());
  }
  repr.push_back('}');

  return scope.Escape(PyString::FromStdString(isolate, repr));
}

}  // namespace saauso::internal
