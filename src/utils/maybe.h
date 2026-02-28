// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_MAYBE_H_
#define SAAUSO_UTILS_MAYBE_H_

#include <cassert>
#include <utility>

namespace saauso::internal {

struct NullMaybeType {};

inline constexpr NullMaybeType kNullMaybe;

template <typename T>
class [[nodiscard]] Maybe {
 public:
  Maybe() = default;
  explicit Maybe(T value) : has_value_(true), value_(value) {}
  Maybe(NullMaybeType) : Maybe() {}

  static Maybe<T> Nothing() { return Maybe<T>(); }

  bool IsNothing() const { return !has_value_; }
  bool IsJust() const { return has_value_; }

  // 便于Maybe适配RETURN_ON_EXCEPTION_VALUE宏
  bool IsEmpty() const { return IsNothing(); }

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

// Maybe<void>特化版本
template <>
class Maybe<void> {
 public:
  constexpr Maybe() = default;
  constexpr Maybe(NullMaybeType) {}

  bool IsNothing() const { return !is_valid_; }
  bool IsEmpty() const { return IsNothing(); }
  bool IsJust() const { return is_valid_; }

  bool operator==(const Maybe& other) const {
    return IsJust() == other.IsJust();
  }

  bool operator!=(const Maybe& other) const { return !operator==(other); }

 private:
  struct JustTag {};

  explicit Maybe(JustTag) : is_valid_(true) {}

  bool is_valid_ = false;

  friend Maybe<void> JustVoid();
};

inline Maybe<void> JustVoid() {
  return Maybe<void>(Maybe<void>::JustTag());
}

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_MAYBE_H_
