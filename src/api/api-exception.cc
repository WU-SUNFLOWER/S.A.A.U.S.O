// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-exception.h"
#include "include/saauso-primitive.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"

namespace saauso {

//////////////////////////////////////////////////////////////////////////////////
// Exception

Local<Value> Exception::TypeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }

  i::Isolate* isolate = i::Isolate::Current();
  return Local<Value>::Cast(String::New(reinterpret_cast<Isolate*>(isolate),
                                        "[TypeError] " + msg->Value()));
}

Local<Value> Exception::RuntimeError(Local<String> msg) {
  if (msg.IsEmpty()) {
    return Local<Value>();
  }

  i::Isolate* isolate = i::Isolate::Current();
  return Local<Value>::Cast(String::New(reinterpret_cast<Isolate*>(isolate),
                                        "[RuntimeError] " + msg->Value()));
}

//////////////////////////////////////////////////////////////////////////////////
// TryCatch

TryCatch::TryCatch(Isolate* isolate) : isolate_(isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  assert(i_isolate == i::Isolate::Current());

  previous_ = i_isolate->try_catch_top();
  i_isolate->set_try_catch_top(this);
}

TryCatch::~TryCatch() {
  Reset();

  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  i_isolate->set_try_catch_top(previous_);
}

bool TryCatch::HasCaught() const {
  return !exception_.IsEmpty();
}

void TryCatch::Reset() {
  exception_ = Local<Value>();
}

Local<Value> TryCatch::Exception() const {
  return exception_;
}

}  // namespace saauso
