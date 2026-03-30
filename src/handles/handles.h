// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLES_H_
#define SAAUSO_HANDLES_HANDLES_H_

#include <cassert>
#include <type_traits>

#include "src/handles/handle-base.h"

namespace saauso::internal {

template <typename T>
class Handle : public HandleBase {
 public:
  Handle() = default;

  // TODO: Handle(Tagged<T> tagged, Isolate* isolate)铺开后，
  //       移除该构造函数！
  explicit Handle(Tagged<T> tagged) : HandleBase(tagged.ptr()) {}
  Handle(Tagged<T> tagged, Isolate* isolate)
      : HandleBase(tagged.ptr(), isolate) {}

  explicit Handle(Address* location) : HandleBase(location) {}

  // 允许Handle的向下转换
  // 例如将Handle<PyList>转换成Handle<PyObject>
  template <typename S>
  constexpr Handle(Handle<S> other)
    requires(is_subtype_v<S, T>)
      : Handle(other.location()) {}

  constexpr T* operator->() const {
    assert(!is_null());
#if defined(_DEBUG) || defined(ASAN_BUILD)
    HandleScope::AssertValidLocation(location());
#endif
    return Tagged<T>(*location()).operator->();
  }

  constexpr Tagged<T> operator*() const {
    if (is_null()) {
      return Tagged<T>::null();
    }
#if defined(_DEBUG) || defined(ASAN_BUILD)
    HandleScope::AssertValidLocation(location());
#endif
    return Tagged<T>(*location());
  }

  template <class S>
  constexpr static Handle<T> cast(Handle<S> that) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
    if (that.is_null()) {
      return Handle<T>::null();
    }
    // 这行代码起到断言的作用
    T::cast(*that);
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)
    return Handle<T>(Tagged<T>::cast(*that));
  }

  constexpr static Handle<T> null() { return Handle<T>(); }
};

// 工具函数，用于方便地将一个裸指针或tagged提升为一个handle
// TODO: handle(Tagged<T> object, Isolate* isolate)铺开后，
//       移除该API
template <typename T>
constexpr Handle<T> handle(Tagged<T> object) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
  if (!object.is_null()) {
    T::cast(object);
  }
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)
  return Handle<T>(object);
}

template <typename T>
constexpr Handle<T> handle(Tagged<T> object, Isolate* isolate) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
  if (!object.is_null()) {
    T::cast(object);
  }
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)
  return Handle<T>(object, isolate);
}

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLES_H_
