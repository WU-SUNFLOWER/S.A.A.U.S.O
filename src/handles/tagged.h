// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_TAGGED_H_
#define SAAUSO_HANDLES_TAGGED_H_

#include <type_traits>

#include "include/saauso-internal.h"

namespace saauso::internal {

/////////////////////////////////// 魔法开始 ///////////////////////////////////
// 参考：
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/objects/tagged.h
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/handles/handles.h
namespace detail {
template <typename Derived, typename Base>
struct is_simple_subtype
    : std::is_same<std::remove_cv_t<Derived>, std::remove_cv_t<Base>> {};

template <typename Derived, typename Base>
struct is_complex_subtype : std::is_base_of<Base, Derived> {};
}  // namespace detail

template <typename Derived, typename Base>
struct is_subtype
    : std::disjunction<detail::is_simple_subtype<Derived, Base>,
                       detail::is_complex_subtype<Derived, Base>> {};

template <typename Derived, typename Base>
static constexpr bool is_subtype_v = is_subtype<Derived, Base>::value;
/////////////////////////////////// 魔法结束 ///////////////////////////////////

template <typename T>
class TaggedBase {
 public:
  explicit TaggedBase(Address address) : address_(address) {}

  bool IsNull() const { return address_ == kNullAddress; }

  Address ptr() const { return address_; }

 protected:
  Address address_;

 private:
  template <typename>
  friend class TaggedBase;
};

template <typename T>
class Tagged : public TaggedBase<T> {
 public:
  Tagged() : TaggedBase<T>(kNullAddress) {}
  explicit Tagged(Address address) : TaggedBase<T>(address) {}
  explicit Tagged(T* address)
      : TaggedBase<T>(reinterpret_cast<Address>(address)) {}

  // 允许子类向父类转换 (Upcast)
  // 例如将 Tagged<PyList> 转换成 Tagged<PyObject>
  template <typename S>
  Tagged(Tagged<S> other)  // NOLINT
    requires(is_subtype_v<S, T>)
      : TaggedBase<T>(other.ptr()) {}

  bool IsSmi() const { return IsSmi(this->ptr()); }

  T* operator->() const {
    assert(!IsSmi());
    return reinterpret_cast<T*>(this->ptr());
  }

  T& operator*() const {
    assert(!IsSmi());
    return reinterpret_cast<T*>(this->ptr());
  }

  static Tagged<T> Null() { return Tagged<T>(); }

  // 警告：Tagged在语义上等价于C++中的裸指针
  // 对它执行Cast，语义相当于C++中的reinterpret_cast！！！
  template <class S>
  static Tagged<T> Cast(Tagged<S> that) {
    return Tagged<T>(reinterpret_cast<T*>(that.ptr()));
  }

 private:
  template <typename>
  friend class Tagged;
};

class PySmi;

template <>
class Tagged<PySmi> : public TaggedBase<PySmi> {
 public:
  Tagged() : TaggedBase<PySmi>(kNullAddress) {}
  explicit Tagged(int64_t value) : TaggedBase<PySmi>(SmiToAddress(value)) {}
  explicit Tagged(Address address) : TaggedBase<PySmi>(address) {}

  bool IsSmi() const { return true; }
  int64_t value() const { return AddressToSmi(this->ptr()); }

  // 禁用指针操作，因为 Smi 不是指针
  PySmi* operator->() const = delete;
  PySmi& operator*() const = delete;
};

}  // namespace saauso::internal

#endif
