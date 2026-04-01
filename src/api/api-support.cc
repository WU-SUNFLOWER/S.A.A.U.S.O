// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>

#include "src/api/api-impl.h"

namespace saauso {
namespace api {


i::Handle<i::PyObject> ToInternalObject(i::Isolate* i_isolate,
                                        Local<Value> value) {
  if (value.IsEmpty()) {
    return i_isolate->factory()->py_none_object();
  }
  i::Handle<i::PyObject> object = internal::Utils::OpenHandle(value);
  if (object.is_null()) {
    return i_isolate->factory()->py_none_object();
  }
  return object;
}

bool CapturePendingException(i::Isolate* isolate) {
  if (!isolate->HasPendingException()) {
    return false;
  }

  TryCatch* try_catch = isolate->try_catch_top();
  if (try_catch == nullptr) {
    return true;
  }

  i::Handle<i::PyObject> exception =
      isolate->exception_state()->pending_exception(isolate);
  ApiAccess::SetTryCatchException(try_catch,
                                  i::Utils::ToLocal<Value>(exception));
  isolate->exception_state()->Clear();
  return true;
}

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
  ApiAccess::SetFunctionCallbackInfoImpl(&callback_info, &callback_info_impl);

  callback(callback_info);

  if (i_isolate->HasPendingException()) {
    return i::kNullMaybeHandle;
  }

  Local<Value> escaped_ret =
      escapable_scope.Escape(callback_info_impl.return_value);
  return ToInternalObject(i_isolate, escaped_ret);
}

}  // namespace api
}  // namespace saauso
