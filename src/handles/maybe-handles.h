// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_MAYBE_HANDLES_H_
#define SAAUSO_HANDLES_MAYBE_HANDLES_H_

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "src/handles/handles.h"

namespace saauso::internal {

struct NullMaybeHandleType {};

inline constexpr NullMaybeHandleType kNullMaybeHandle;

// 用于表示“可能成功也可能失败”的 Handle 返回值。
// 该类型本身不携带错误信息，错误信息由Isolate的异常状态容器统一持有与传播。
template <typename T>
class MaybeHandle {
 public:
  MaybeHandle() = default;

  explicit MaybeHandle(NullMaybeHandleType) : MaybeHandle() {}

  // 支持将Handle向下转换为MaybeHandle
  // 例如将Handle<PyList>转换成MaybeHandle<PyObject>
  template <typename S>
  constexpr MaybeHandle(Handle<S> handle)
    requires(is_subtype_v<S, T>)
      : MaybeHandle(handle.location()) {}

  // 支持MaybeHandle的向下转换
  // 例如将Handle<PyList>转换成MaybeHandle<PyObject>
  template <typename S>
  constexpr MaybeHandle(MaybeHandle<S> maybe_handle)
    requires(is_subtype_v<S, T>)
      : MaybeHandle(maybe_handle.location()) {}

  explicit MaybeHandle(Tagged<T> tagged) : MaybeHandle(handle(tagged)) {};

  void Check() const {
    if (is_null()) {
      std::fprintf(stderr, "fatal null MaybeHandle");
      std::exit(1);
    }
  }

  Handle<T> ToHandleChecked() const {
    Check();
    return Handle<T>(location_);
  }

  // 常规API: 将MaybeHandle转换为普通Handle。
  // 调用该API时，要求T可以向下转换为S，否则编译阶段即会报错！
  template <typename S>
  bool ToHandle(Handle<S>* out) const {
    if (is_null()) {
      *out = Handle<T>::null();
      return false;
    } else {
      *out = Handle<T>(location_);
      return true;
    }
  }

  constexpr bool is_null() const { return location_ == nullptr; }
  constexpr static MaybeHandle<T> null() { return MaybeHandle<T>(); }

  constexpr Address* location() const { return location_; }

  // 快速检测两个MaybeHandle是否指向同一个对象
  constexpr bool is_identical_to(const MaybeHandle<T> other) const {
    return location() == other.location() || operator*() == *other;
  }

 private:
  template <typename>
  friend class MaybeHandle;

  Address* location_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_MAYBE_HANDLES_H_
