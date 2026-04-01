// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cassert>

#include "include/saauso-isolate.h"
#include "include/saauso-primitive.h"
#include "src/api/api-support.h"

namespace saauso {

Isolate* Isolate::New(const IsolateCreateParams&) {
  i::Isolate* i_isolate = i::Isolate::New();
  return reinterpret_cast<Isolate*>(i_isolate);
}

void Isolate::Enter() {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(this);
  i_isolate->Enter();
}

void Isolate::Exit() {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(this);
  i_isolate->Exit();
}

void Isolate::Dispose() {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(this);
  i::Isolate::Dispose(i_isolate);
}

void Isolate::ThrowException(Local<Value> exception) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(this);
  assert(i_isolate != nullptr);

  if (i_isolate == nullptr) {
    return;
  }

  i::Handle<i::PyObject> py_exception = api::Utils::OpenHandle(exception);
  assert(!py_exception.is_null());

  i_isolate->exception_state()->Throw(*py_exception);
  api::CapturePendingException(i_isolate);
}

size_t Isolate::ValueRegistrySizeForTesting() const {
  return 0;
}

}  // namespace saauso
