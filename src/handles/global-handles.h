// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_GLOBAL_HANDLES_H_
#define SAAUSO_HANDLES_GLOBAL_HANDLES_H_

#include <cassert>

#include "src/handles/handle_scope_implementer.h"
#include "src/handles/handles.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

template <typename T>
class Global {
 public:
  Global() = default;

  explicit Global(Isolate* isolate, Tagged<T> object) : isolate_(isolate) {
    assert(isolate_ != nullptr);
    location_ = isolate_->handle_scope_implementer()->CreateGlobalHandle(
        Tagged<PyObject>(object).ptr());
  }

  explicit Global(Tagged<T> object) : Global(Isolate::Current(), object) {}

  template <typename S>
  Global(Handle<S> local)  // NOLINT
    requires(is_subtype_v<S, T>)
      : Global(Isolate::Current(), Tagged<T>(*local)) {}

  // 类似于std::unique_ptr，禁止拷贝和赋值，避免对应的slot槽位被重复释放
  Global(const Global&) = delete;
  Global& operator=(const Global&) = delete;

  Global(Global&& other) noexcept { MoveFrom(std::move(other)); }

  template <typename S>
  Global(Global<S>&& other) noexcept  // NOLINT
    requires(is_subtype_v<S, T>)
      : isolate_(other.isolate_), location_(other.location_) {
    other.isolate_ = nullptr;
    other.location_ = nullptr;
  }

  Global& operator=(Global&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    Reset();
    MoveFrom(std::move(other));
    return *this;
  }

  ~Global() { Reset(); }

  bool IsEmpty() const { return location_ == nullptr; }

  void Reset() {
    if (location_ == nullptr) {
      isolate_ = nullptr;
      return;
    }
    assert(isolate_ != nullptr);
    // 释放slot槽位
    isolate_->handle_scope_implementer()->DestroyGlobalHandle(location_);
    isolate_ = nullptr;
    location_ = nullptr;
  }

  void Reset(Tagged<T> object) {
    if (location_ == nullptr) {
      isolate_ = Isolate::Current();
      location_ = isolate_->handle_scope_implementer()->CreateGlobalHandle(
          Tagged<PyObject>(object).ptr());
      return;
    }
    assert(isolate_ == Isolate::Current());
    *location_ = Tagged<PyObject>(object).ptr();
  }

  Handle<T> Get() const {
    if (IsEmpty()) {
      return Handle<T>::null();
    }
    assert(isolate_ == Isolate::Current());
    return Handle<T>(GetDirectionPtr());
  }

  Isolate* isolate() const { return isolate_; }

 private:
  template <typename>
  friend class Global;

  void MoveFrom(Global&& other) {
    isolate_ = other.isolate_;
    location_ = other.location_;
    other.isolate_ = nullptr;
    other.location_ = nullptr;
  }

  Tagged<T> GetDirectionPtr() const {
    if (IsEmpty()) {
      return Tagged<T>::null();
    }
    return Tagged<T>(*location_);
  }

  Isolate* isolate_{nullptr};
  Address* location_{nullptr};  // 指向slot的指针
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_GLOBAL_HANDLES_H_
