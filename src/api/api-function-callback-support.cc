// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/api/api-function-callback-support.h"

#include "include/saauso-function-callback.h"
#include "src/api/api-bridge-access.h"
#include "src/api/api-handle-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-smi.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso {
namespace api {

i::MaybeHandle<i::PyObject> InvokeEmbedderCallback(
    i::Isolate* i_isolate,
    i::Handle<i::PyObject> receiver,
    i::Handle<i::PyTuple> args,
    i::Handle<i::PyDict>,
    i::Handle<i::PyObject> closure_data) {
  if (closure_data.is_null() || !i::IsPySmi(closure_data)) {
    i::Runtime_ThrowError(i_isolate, i::ExceptionType::kRuntimeError,
                          "Embedder callback closure_data is invalid");
    return i::kNullMaybeHandle;
  }
  int64_t callback_addr =
      i::PySmi::ToInt(i::Tagged<i::PySmi>::cast(*closure_data));
  FunctionCallback callback =
      reinterpret_cast<FunctionCallback>(static_cast<intptr_t>(callback_addr));
  if (callback == nullptr) {
    i::Runtime_ThrowError(i_isolate, i::ExceptionType::kRuntimeError,
                          "Embedder callback binding is missing");
    return i::kNullMaybeHandle;
  }
  EscapableHandleScope escapable_scope(reinterpret_cast<Isolate*>(i_isolate));

  FunctionCallbackInfoImpl callback_info_impl;
  callback_info_impl.isolate = i_isolate;
  callback_info_impl.receiver = receiver;
  callback_info_impl.args = args;

  FunctionCallbackInfo callback_info;
  ApiBridgeAccess::SetFunctionCallbackInfoImpl(&callback_info,
                                               &callback_info_impl);

  callback(callback_info);

  if (i_isolate->HasPendingException()) {
    return i::kNullMaybeHandle;
  }

  Local<Value> escaped_ret =
      escapable_scope.Escape(callback_info_impl.return_value);
  return api::Utils::OpenHandle(escaped_ret);
}

}  // namespace api
}  // namespace saauso
