// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-exception.h"
#include "include/saauso-primitive.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/execution/exception-types.h"
#include "src/handles/global-handles.h"
#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso {

struct TryCatch::Impl {
  i::Global<i::PyObject> exception_;
  int python_execution_depth_{0};
};

//////////////////////////////////////////////////////////////////////////////////
// Exception

Local<Value> Exception::TypeError(Local<String> message) {
  assert(!message.IsEmpty());

  i::Isolate* isolate = api::RequireCurrentIsolate();
  i::Handle<i::PyString> py_message =
      i::Handle<i::PyString>::cast(api::Utils::OpenHandle(message));
  i::Handle<i::PyBaseException> error =
      isolate->factory()->NewExceptionFromMessage(i::ExceptionType::kTypeError,
                                                  py_message);
  return api::Utils::ToLocal<Value>(error);
}

Local<Value> Exception::RuntimeError(Local<String> message) {
  assert(!message.IsEmpty());

  i::Isolate* isolate = api::RequireCurrentIsolate();
  i::Handle<i::PyString> py_message =
      i::Handle<i::PyString>::cast(api::Utils::OpenHandle(message));
  i::Handle<i::PyBaseException> error =
      isolate->factory()->NewExceptionFromMessage(
          i::ExceptionType::kRuntimeError, py_message);
  return api::Utils::ToLocal<Value>(error);
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

Local<String> TryCatch::Message() const {
  i::Handle<i::PyObject> raw_exception = impl_->exception_.Get(i_isolate_);
  if (raw_exception.is_null()) {
    return Local<String>();
  }

  auto exception = i::Handle<i::PyBaseException>::cast(raw_exception);
  i::Handle<i::PyString> message;
  if (!i::Runtime_ParseExceptionMessageFromArgs(i_isolate_, exception)
           .ToHandle(&message)) {
    return Local<String>();
  }

  return api::Utils::ToLocal<String>(message);
}

}  // namespace saauso
