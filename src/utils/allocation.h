// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SSAUSO_UTILS_ALLOCATION_H_
#define SSAUSO_UTILS_ALLOCATION_H_

#include <cstdio>
#include <cstdlib>
#include <new>

namespace saauso::internal {

template <typename T>
T* NewArray(size_t length) {
  T* result = new (std::nothrow) T[length];
  if (result == nullptr) [[unlikely]] {
    std::printf("VM OOM\n");
    std::exit(1);
  }
  return result;
}

template <typename T>
T* NewArray(size_t length, T default_val) {
  T* result = NewArraym<T>(length);
  for (size_t i = 0; i < length; ++i) {
    result[i] = default_val;
  }
  return result;
}

template <typename T>
void DeleteArray(T* array) {
  delete[] array;
}

}  // namespace saauso::internal

#endif  // SSAUSO_UTILS_ALLOCATION_H_
