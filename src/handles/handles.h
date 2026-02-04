// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLES_H_
#define SAAUSO_HANDLES_HANDLES_H_

#include <cassert>
#include <type_traits>

#include "src/handles/tagged.h"

namespace saauso::internal {

class Isolate;
class HandleScopeImplementer;

class HandleScope {
 public:
  struct State {
    Address* next{nullptr};
    Address* limit{nullptr};
    int extension{-1};  // 进入该scope后额外申请的block数量
  };

  HandleScope();
  HandleScope(const HandleScope&) = delete;
  HandleScope& operator=(const HandleScope&) = delete;

  ~HandleScope();

 protected:
  Address* CloseAndEscape(Address ptr);

 private:
  friend class HandleScopeImplementer;

  template <typename>
  friend class Handle;

  static void Extend();

  static Address* CreateHandle(Address ptr);

  static void AssertValidLocation(Address* location);

  void Close();

  Isolate* isolate_{nullptr};
  State previous_;
  bool is_closed_{false};
};

template <typename T>
class Handle;

class EscapableHandleScope final : public HandleScope {
 public:
  EscapableHandleScope() = default;
  EscapableHandleScope(const EscapableHandleScope&) = delete;
  EscapableHandleScope& operator=(const EscapableHandleScope&) = delete;

  template <typename T>
  Handle<T> Escape(Handle<T> value) {
    return value.is_null() ? Handle<T>::null()
                           : Handle<T>(CloseAndEscape(value.location_));
  }
};

template <typename T>
class Handle {
 public:
  Handle() = default;
  explicit Handle(Address* location) : location_(location) {}
  explicit Handle(Tagged<T> tagged) {
    if (!tagged.is_null()) {
      location_ = HandleScope::CreateHandle(tagged.ptr());
    }
  }

  // 允许Handle的向下转换
  // 例如将Handle<PyList>转换成Handle<PyObject>
  template <typename S>
  Handle(Handle<S> other)  // NOLINT
    requires(is_subtype_v<S, T>)
      : Handle(other.location()) {}

  T* operator->() const {
    assert(!is_null());
#if defined(_DEBUG) || defined(ASAN_BUILD)
    HandleScope::AssertValidLocation(location_);
#endif
    return Tagged<T>(*location_).operator->();
  }

  Tagged<T> operator*() const {
    if (is_null()) {
      return Tagged<T>::null();
    }
#if defined(_DEBUG) || defined(ASAN_BUILD)
    HandleScope::AssertValidLocation(location_);
#endif
    return Tagged<T>(*location_);
  }

  template <class S>
  static Handle<T> cast(Handle<S> that) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
    if (that.is_null()) {
      return Handle<T>::null();
    }
    T::cast(*that);
#endif  // 这行代码起到断言的作用
    return Handle<T>(Tagged<T>::cast(*that));
  }

  constexpr bool is_null() const { return location_ == nullptr; }
  static Handle<T> null() { return Handle<T>(); }

  Address* location() const { return location_; }

  // 快速检测两个handle是否指向同一个对象
  bool is_identical_to(const Handle<T> other) const {
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
  if (!object.is_null()) {
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
