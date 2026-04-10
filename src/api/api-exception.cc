// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-exception.h"
#include "include/saauso-primitive.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/handles/global-handles.h"
#include "src/objects/py-object.h"

namespace saauso {

struct TryCatch::Impl {
  i::Global<i::PyObject> exception_;
  int python_execution_depth_{0};
};

//////////////////////////////////////////////////////////////////////////////////
// Exception

MaybeLocal<Value> Exception::TypeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Isolate* isolate = api::RequireCurrentIsolate();
  Local<String> out = String::New(reinterpret_cast<Isolate*>(isolate),
                                  "[TypeError] " + msg->Value());
  return out;
}

MaybeLocal<Value> Exception::RuntimeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return MaybeLocal<Value>();
  }

  i::Isolate* isolate = api::RequireCurrentIsolate();
  Local<String> out = String::New(reinterpret_cast<Isolate*>(isolate),
                                  "[RuntimeError] " + msg->Value());
  return out;
}

//////////////////////////////////////////////////////////////////////////////////
// TryCatch

TryCatch::TryCatch(Isolate* isolate)
    : i_isolate_(api::RequireExplicitIsolate(isolate)),
      impl_(std::make_unique<Impl>()) {
  impl_->python_execution_depth_ = i_isolate_->python_execution_depth();
  previous_ = i_isolate_->try_catch_top();
  i_isolate_->set_try_catch_top(this);
}

TryCatch::~TryCatch() {
  Reset();
  i_isolate_->set_try_catch_top(previous_);
}

bool TryCatch::HasCaught() const {
  return !impl_->exception_.IsEmpty();
}

void TryCatch::SetException(i::Handle<i::PyObject> exception) {
  impl_->exception_ = i::Global<i::PyObject>(i_isolate_, exception);
}

void TryCatch::Reset() {
  impl_->exception_.Reset();
}

int TryCatch::PythonExecutionDepth() const {
  return impl_->python_execution_depth_;
}

Local<Value> TryCatch::Exception() const {
  i::Handle<i::PyObject> exception = impl_->exception_.Get(i_isolate_);
  if (exception.is_null()) {
    return Local<Value>();
  }
  return api::Utils::ToLocal<Value>(exception);
}

}  // namespace saauso
