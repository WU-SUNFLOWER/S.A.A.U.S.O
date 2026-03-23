// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-local-handle.h"
#include "include/saauso-value.h"
#include "src/api/api-impl.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso {

namespace api {

// TODO:
// 由于目前虚拟机内部的 HandleScope 在创建时通过调用 Isolate::Current()
// 隐式获取当前 isolate，并不受显式传入的 isolate 驱动，
// 这里为了保证一致性，我们临时添加一个断言校验。
// 在虚拟机内部 HandleScope 完成显式接收 isolate 改造后，此处断言可以移除！
struct HandleScopeImpl {
  explicit HandleScopeImpl(i::Isolate* isolate) : isolate(isolate) {
    assert(isolate == i::Isolate::Current());
  }

  i::Isolate* isolate{nullptr};
  i::HandleScope handle_scope;
};

struct EscapableHandleScopeImpl {
  explicit EscapableHandleScopeImpl(i::Isolate* isolate) : isolate(isolate) {
    assert(isolate == i::Isolate::Current());
  }

  i::Isolate* isolate{nullptr};
  i::EscapableHandleScope handle_scope;
};

}  // namespace api

HandleScope::HandleScope(Isolate* isolate) {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  impl_ = new api::HandleScopeImpl(i_isolate);
}

HandleScope::~HandleScope() {
  delete reinterpret_cast<api::HandleScopeImpl*>(impl_);
  impl_ = nullptr;
}

EscapableHandleScope::EscapableHandleScope(Isolate* isolate) {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
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
      impl->handle_scope.Escape(i::Utils::OpenHandle(value));
  return i::Utils::ToLocal<Value>(escaped);
}

}  // namespace saauso
