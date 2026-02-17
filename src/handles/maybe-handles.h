// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_MAYBE_HANDLES_H_
#define SAAUSO_HANDLES_MAYBE_HANDLES_H_

#include <cassert>

#include "src/handles/handles.h"

namespace saauso::internal {

// 用于表示“可能成功也可能失败”的 Handle 返回值。
// 该类型本身不携带错误信息，错误信息应由解释器的异常状态容器统一持有与传播。
template <typename T>
class MaybeHandle {
 public:
  MaybeHandle() = default;
  MaybeHandle(const MaybeHandle&) = default;
  MaybeHandle& operator=(const MaybeHandle&) = default;

  explicit MaybeHandle(Handle<T> handle) : handle_(handle) {}

  bool is_null() const { return handle_.is_null(); }

  Handle<T> ToHandleChecked() const {
    assert(!is_null());
    return handle_;
  }

  Handle<T> ToHandleNoThrow() const { return handle_; }

  static MaybeHandle<T> Null() { return MaybeHandle<T>(); }

 private:
  Handle<T> handle_{Handle<T>::null()};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_MAYBE_HANDLES_H_

