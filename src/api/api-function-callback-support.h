// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_FUNCTION_CALLBACK_SUPPORT_H_
#define SAAUSO_API_API_FUNCTION_CALLBACK_SUPPORT_H_

#include "include/saauso-local-handle.h"
#include "src/handles/maybe-handles.h"

namespace saauso {

namespace internal {
class PyObject;
class PyTuple;
class PyDict;
}  // namespace internal

namespace api {

struct FunctionCallbackInfoImpl {
  i::Isolate* isolate{nullptr};
  i::Handle<i::PyObject> receiver;
  i::Handle<i::PyTuple> args;
  Local<Value> return_value;
};

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* internal_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict> kwargs,
    i::Handle<i::PyObject> closure_data);

}  // namespace api
}  // namespace saauso

#endif
