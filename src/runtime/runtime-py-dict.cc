// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-dict.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

MaybeHandle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                      Handle<PyObject> kwargs) {
  EscapableHandleScope scope;
  auto* isolate = Isolate::Current();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc > 1) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "dict expected at most 1 argument, got %lld",
                        static_cast<long long>(argc));
    return kNullMaybeHandle;
  }

  Handle<PyDict> result = PyDict::NewInstance();

  if (argc == 1) {
    Handle<PyObject> input = pos_args->Get(0);
    if (IsPyDict(input)) {
      result = PyDict::Clone(Handle<PyDict>::cast(input));
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
                              "element #%lld to a sequence",
                              static_cast<long long>(i));
          return kNullMaybeHandle;
        }

        PyDict::Put(result, pair->Get(0), pair->Get(1));
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
            return kNullMaybeHandle;
          }
          key = key_str;
        }

        PyDict::Put(result, key, item->Get(1));
      }
    }
  }

  return scope.Escape(result);
}

}  // namespace saauso::internal
