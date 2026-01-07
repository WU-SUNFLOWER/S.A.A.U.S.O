// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLES_H_
#define SAAUSO_HANDLES_HANDLES_H_

#include <cassert>
#include <type_traits>

#include "src/objects/objects.h"

namespace saauso::internal {

/////////////////////////////////// 魔法开始 ///////////////////////////////////
// 参考：
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/objects/tagged.h
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/handles/handles.h
namespace anonymous {
template <typename Derived, typename Base>
struct is_simple_subtype
    : std::is_same<std::remove_cv_t<Derived>, std::remove_cv_t<Base>> {};

template <typename Derived, typename Base>
struct is_complex_subtype : std::is_base_of<Base, Derived> {};
}  // namespace anonymous

template <typename Derived, typename Base>
struct is_subtype
    : std::disjunction<anonymous::is_simple_subtype<Derived, Base>,
                       anonymous::is_complex_subtype<Derived, Base>> {};

template <typename Derived, typename Base>
static constexpr bool is_subtype_v = is_subtype<Derived, Base>::value;
/////////////////////////////////// 魔法结束 ///////////////////////////////////

class HandleScope;

template <typename T>
class Handle {
 public:
  Handle() : address_(nullptr) {}
  explicit Handle(T* address) : address_(static_cast<Object*>(address)) {}

  // 允许Handle的向下转换
  // 例如将Handle<PyList>转换成Handle<PyObject>
  template <typename S>
  Handle(Handle<S> other)  // NOLINT
    requires(is_subtype_v<S, T>)
      : address_(other.address_) {}

  T* operator->() const {
    assert(!IsNull());
    return reinterpret_cast<T*>(address_);
  }

  T* operator*() const {
    assert(!IsNull());
    return reinterpret_cast<T*>(address_);
  }

  template <class S>
  static Handle<T> Cast(Handle<S> that) {
    T::Cast(*that);  // 这行代码起到断言的作用
    return Handle<T>(reinterpret_cast<T*>(*that));
  }

  bool IsNull() const { return address_ == nullptr; }
  static Handle<T> Null() { return Handle<T>(); }

  Handle<T> EscapeFrom(HandleScope* scope) {
    return Handle<T>(reinterpret_cast<T*>(address_));
  }

 private:
  // 为了便于隐式转换，
  // 我们允许任意类型的Handle<T>可以访问其他任意类型Handle<S>的内部指针
  template <typename>
  friend class Handle;

  Object* address_;
};

class [[maybe_unused]] HandleScope {};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLES_H_
