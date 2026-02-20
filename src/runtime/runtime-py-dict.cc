// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-dict.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Handle<PyObject> Runtime_NewDict(Handle<PyObject> args,
                                 Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc > 1) {
    std::fprintf(stderr,
                 "TypeError: dict expected at most 1 argument, got %lld\n",
                 static_cast<long long>(argc));
    std::exit(1);
  }

  Handle<PyDict> result = PyDict::NewInstance();

  if (argc == 1) {
    Handle<PyObject> input = pos_args->Get(0);
    if (IsPyDict(input)) {
      result = PyDict::Clone(Handle<PyDict>::cast(input));
    } else {
      Handle<PyTuple> elements = Runtime_UnpackIterableObjectToTuple(input);
      for (int64_t i = 0; i < elements->length(); ++i) {
        Handle<PyObject> elem = elements->Get(i);
        Handle<PyTuple> pair = Runtime_UnpackIterableObjectToTuple(elem);
        if (pair->length() != 2) {
          std::fprintf(stderr,
                       "TypeError: cannot convert dictionary update sequence "
                       "element #%lld to a sequence\n",
                       static_cast<long long>(i));
          std::exit(1);
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
          key = Runtime_NewStr(key);
        }

        PyDict::Put(result, key, item->Get(1));
      }
    }
  }

  return scope.Escape(result);
}

}  // namespace saauso::internal
