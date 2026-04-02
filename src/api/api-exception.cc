// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-exception.h"
#include "include/saauso-primitive.h"
#include "src/api/api-isolate-utils.h"

namespace saauso {

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
    : i_isolate_(api::RequireExplicitIsolate(isolate)) {
  previous_ = i_isolate_->try_catch_top();
  i_isolate_->set_try_catch_top(this);
}

TryCatch::~TryCatch() {
  Reset();
  i_isolate_->set_try_catch_top(previous_);
}

bool TryCatch::HasCaught() const {
  return !exception_.IsEmpty();
}

void TryCatch::Reset() {
  exception_ = Local<Value>();
}

MaybeLocal<Value> TryCatch::Exception() const {
  return exception_;
}

}  // namespace saauso
