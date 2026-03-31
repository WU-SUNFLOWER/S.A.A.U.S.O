// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_GLOBAL_HANDLES_H_
#define SAAUSO_HANDLES_GLOBAL_HANDLES_H_

#include <cassert>

#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"

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

  template <typename S>
  Global(Isolate* isolate, Handle<S> local)  // NOLINT
    requires(is_subtype_v<S, T>)
      : Global(isolate, Tagged<T>(*local)) {}

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
    // 该 Global 句柄有申请实际的 slot 槽位，那么需要首先进行释放
    if (location_ != nullptr) {
      assert(isolate_ != nullptr);
      isolate_->handle_scope_implementer()->DestroyGlobalHandle(location_);
    }
    isolate_ = nullptr;
    location_ = nullptr;
  }

  void Reset(Isolate* isolate, Tagged<T> object) {
    Reset();
    isolate_ = isolate;
    location_ = isolate_->handle_scope_implementer()->CreateGlobalHandle(
        Tagged<PyObject>(object).ptr());
  }

  Handle<T> Get(Isolate* isolate) const {
    if (IsEmpty()) {
      return Handle<T>::null();
    }
    assert(isolate_ == isolate);
    return Handle<T>(GetDirectionPtr(), isolate);
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
