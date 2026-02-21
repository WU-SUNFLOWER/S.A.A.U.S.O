// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_UTILS_MAYBE_H_
#define SRC_UTILS_MAYBE_H_

#include <cassert>
#include <utility>

namespace saauso::internal {

template <typename T>
class [[nodiscard]] Maybe {
 public:
  Maybe() = default;
  explicit Maybe(T value) : has_value_(true), value_(value) {}

  static Maybe<T> Nothing() { return Maybe<T>(); }

  bool IsNothing() const { return !has_value_; }
  bool IsJust() const { return has_value_; }

  bool To(T* out) const {
    if (!has_value_) {
      return false;
    }
    *out = value_;
    return true;
  }

  bool MoveTo(T* out) && {
    if (!has_value_) {
      return false;
    }
    *out = std::move(value_);
    return true;
  }

  T ToChecked() const {
    assert(has_value_);
    return value_;
  }

 private:
  bool has_value_{false};
  T value_{};
};

}  // namespace saauso::internal

#endif  // SRC_UTILS_MAYBE_H_
