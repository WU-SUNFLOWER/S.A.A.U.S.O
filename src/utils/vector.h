// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_VECTOR_H_
#define SAAUSO_UTILS_VECTOR_H_

#include <algorithm>
#include <cassert>

#include "src/utils/allocation.h"

namespace saauso::internal {

template <typename T>
class Vector {
 public:
  static constexpr size_t kMinimumCapacity = 8;

  Vector() = default;
  ~Vector() { DeleteArray<T>(data_); }

  T PopBack() { return data_[--length_]; }

  void PushBack(T elem) {
    if (length_ == capacity_) {
      size_t new_capacity = std::max(kMinimumCapacity, capacity_ << 1);
      Resize(new_capacity);
    }
    data_[length_++] = elem;
  }

  T& Get(size_t i) const {
    assert(i < length_);
    return data_[i];
  }

  T& GetFront() const {
    assert(!IsEmpty());
    return data_[0];
  }

  T& GetBack() const {
    assert(!IsEmpty());
    return data_[length_ - 1];
  }

  void Set(size_t i, T elem) {
    assert(i < length_);
    data_[i] = elem;
  }

  void ShrinkToFit() {
    size_t new_capacity = std::max(length_, kMinimumCapacity);
    if (new_capacity < (capacity_ >> 1)) {
      Resize(new_capacity);
    }
  }

  size_t length() const { return length_; }
  size_t capacity() const { return capacity_; }
  bool IsEmpty() const { return length_ == 0; }

 private:
  void Resize(size_t new_capacity) {
    assert(length_ <= new_capacity);
    T* new_data = NewArray<T>(new_capacity);

    std::copy(data_, data_ + length_, new_data);
    DeleteArray<T>(data_);

    data_ = new_data;
    capacity_ = new_capacity;
  }

  T* data_{nullptr};
  size_t length_{0};
  size_t capacity_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_VECTOR_H_
