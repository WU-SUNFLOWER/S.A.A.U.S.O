// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-local-handle.h"
#include "include/saauso-value.h"
#include "src/api/api-handle-utils.h"
#include "src/api/api-isolate-utils.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso {

namespace api {

struct HandleScopeImpl {
  explicit HandleScopeImpl(i::Isolate* isolate) : handle_scope(isolate) {}

  i::HandleScope handle_scope;
};

struct EscapableHandleScopeImpl {
  explicit EscapableHandleScopeImpl(i::Isolate* isolate)
      : handle_scope(isolate) {}

  i::EscapableHandleScope handle_scope;
};

}  // namespace api

HandleScope::HandleScope(Isolate* isolate) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);
  impl_ = new api::HandleScopeImpl(i_isolate);
}

HandleScope::~HandleScope() {
  delete reinterpret_cast<api::HandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

EscapableHandleScope::EscapableHandleScope(Isolate* isolate) {
  i::Isolate* i_isolate = api::RequireExplicitIsolate(isolate);
  impl_ = new api::EscapableHandleScopeImpl(i_isolate);
}

EscapableHandleScope::~EscapableHandleScope() {
  delete reinterpret_cast<api::EscapableHandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

Local<Value> EscapableHandleScope::Escape(Local<Value> value) {
  if (value.IsEmpty()) {
    return value;
  }
  auto* impl = reinterpret_cast<api::EscapableHandleScopeImpl*>(impl_);
  if (impl == nullptr) {
    return Local<Value>();
  }
  i::Handle<i::PyObject> escaped =
      impl->handle_scope.Escape(api::Utils::OpenHandle(value));
  return api::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
