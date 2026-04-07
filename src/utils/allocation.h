// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_UTILS_ALLOCATION_H_
#define SAAUSO_UTILS_ALLOCATION_H_

#include <cstdio>
#include <cstdlib>
#include <new>

namespace saauso::internal {

//////////////////////////////////////////////////////////////
// 分配简单的数组

template <typename T>
T* NewArray(size_t length) {
  T* result = new (std::nothrow) T[length];
  if (result == nullptr) [[unlikely]] {
    std::fprintf(stderr, "VM OOM\n");
    std::exit(1);
  }
  return result;
}

template <typename T>
T* NewArray(size_t length, T default_val) {
  T* result = NewArray<T>(length);
  for (size_t i = 0; i < length; ++i) {
    result[i] = default_val;
  }
  return result;
}

template <typename T>
void DeleteArray(T* array) {
  delete[] array;
}

//////////////////////////////////////////////////////////////
// 分配大块虚拟内存

class VirtualMemory {
 public:
  // Reserves virtual memory with size.
  explicit VirtualMemory(size_t size);
  VirtualMemory(size_t size, size_t alignment);
  ~VirtualMemory();

  // Returns whether the memory has been reserved.
  bool IsReserved();

  // Returns the start address of the reserved memory.
  void* address();

  // Returns the size of the reserved memory.
  size_t size() { return size_; }

  // Commits real memory. Returns whether the operation succeeded.
  bool Commit(void* address, size_t size, bool is_executable);

  // Uncommit real memory.  Returns whether the operation succeeded.
  bool Uncommit(void* address, size_t size);

 private:
  void Reserve(size_t size, size_t alignment);

  void* address_;  // Start address of the virtual memory.
  size_t size_;    // Size of the virtual memory.
};

}  // namespace saauso::internal

#endif  // SAAUSO_UTILS_ALLOCATION_H_
