// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLES_H_
#define SAAUSO_HANDLES_HANDLES_H_

#include <cassert>
#include <type_traits>

#include "src/handles/tagged.h"

namespace saauso::internal {

class HandleScopeImplementer;

class HandleScope {
 public:
  struct State {
    Address* next{nullptr};
    Address* limit{nullptr};
    int extension{-1};  // 进入该scope后额外申请的block数量
  };

  HandleScope();
  ~HandleScope();

 private:
  friend class HandleScopeImplementer;

  template <typename>
  friend class Handle;

  static void Extend();

  static Address* CreateHandle(Address ptr);

  void Close();

  Address* EscapeFromSelf(Address ptr);

  static thread_local State current_;

  State previous_;
  bool is_closed_{false};
};

template <typename T>
class Handle {
 public:
  Handle() = default;
  explicit Handle(Address* location) : location_(location) {}
  explicit Handle(Tagged<T> tagged) {
    if (!tagged.IsNull()) {
      location_ = HandleScope::CreateHandle(tagged.ptr());
    }
  }

  // 允许Handle的向下转换
  // 例如将Handle<PyList>转换成Handle<PyObject>
  template <typename S>
  Handle(Handle<S> other)  // NOLINT
    requires(is_subtype_v<S, T>)
      : Handle(other.location()) {}

  constexpr T* operator->() const {
    assert(!IsNull());
    return Tagged<T>(*location_).operator->();
  }

  constexpr Tagged<T> operator*() const {
    return IsNull() ? Tagged<T>::Null() : Tagged<T>(*location_);
  }

  template <class S>
  static Handle<T> cast(Handle<S> that) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
    if (that.IsNull()) {
      return Handle<T>::Null();
    }
    T::cast(*that);
#endif  // 这行代码起到断言的作用
    return Handle<T>(Tagged<T>::cast(*that));
  }

  constexpr bool IsNull() const { return location_ == nullptr; }
  static Handle<T> Null() { return Handle<T>(); }

  Handle<T> EscapeFrom(HandleScope* scope) {
    return Handle<T>(scope->EscapeFromSelf(*location_));
  }

  constexpr Address* location() const { return location_; }

  // 快速检测两个handle是否指向同一个对象
  constexpr bool is_identical_to(const Handle<T> other) const {
    return location() == other.location() || operator*() == *other;
  }

 private:
  // 为了便于隐式转换，
  // 我们允许任意类型的Handle<T>可以访问其他任意类型Handle<S>的内部指针
  template <typename>
  friend class Handle;

  Address* location_{nullptr};
};

// 工具函数，用于方便地将一个裸指针或tagged提升为一个handle
template <typename T>
Handle<T> handle(Tagged<T> object) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
  if (!object.IsNull()) {
    T::cast(object);
  }
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)
  return Handle<T>(object);
}

template <typename T>
Handle<T> handle(T object) {
  return handle(Tagged<T>(object));
}

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLES_H_
