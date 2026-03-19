// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-embedder.h"
#include "src/api/api-inl.h"
#include "src/handles/handle-scopes.h"

namespace saauso {

namespace {

struct HandleScopeImpl {
  explicit HandleScopeImpl(internal::Isolate* isolate)
      : isolate_scope(isolate) {}

  internal::Isolate::Scope isolate_scope;
  internal::HandleScope handle_scope;
};

}  // namespace

Isolate* Isolate::New(const IsolateCreateParams&) {
  internal::Isolate* internal_isolate = internal::Isolate::New();
  if (internal_isolate == nullptr) {
    return nullptr;
  }
  auto* isolate = new Isolate();
  isolate->internal_isolate_ = internal_isolate;
  isolate->default_context_ = new Context(isolate);
  return isolate;
}

void Isolate::Dispose() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(this);
  delete default_context_;
  default_context_ = nullptr;
  internal::Isolate::Dispose(internal_isolate);
  internal_isolate_ = nullptr;
  delete this;
}

HandleScope::HandleScope(Isolate* isolate) {
  impl_ = new HandleScopeImpl(ApiAccess::UnwrapIsolate(isolate));
}

HandleScope::~HandleScope() {
  delete reinterpret_cast<HandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

Local<Context> Context::New(Isolate* isolate) {
  if (isolate == nullptr) {
    return Local<Context>();
  }
  return Local<Context>(isolate->default_context_);
}

void Context::Enter() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(isolate_);
  if (internal_isolate != nullptr) {
    internal_isolate->Enter();
  }
}

void Context::Exit() {
  auto* internal_isolate = ApiAccess::UnwrapIsolate(isolate_);
  if (internal_isolate != nullptr) {
    internal_isolate->Exit();
  }
}

ContextScope::ContextScope(Local<Context> context) : context_(context) {
  if (!context_.IsEmpty()) {
    context_->Enter();
  }
}

ContextScope::~ContextScope() {
  if (!context_.IsEmpty()) {
    context_->Exit();
  }
}

}  // namespace saauso
