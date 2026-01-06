// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_HEAP_H_
#define SAAUSO_HEAP_HEAP_H_

namespace saauso::internal {

class Heap {
 public:
  enum class AllocationSpace { kNewSpace, kOldSpace, kMetaSpace };

  void* AllocateRaw(size_t size_in_bytes, AllocationSpace space);

  template <typename T>
  T* Allocate(AllocationSpace space) {
    return static_cast<T*>(AllocateRaw(sizeof(T), space));
  }
};

#define WRITE_BARRIER

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_HEAP_H_
