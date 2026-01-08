// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"

#include <cstdlib>
#include <cstring>
#include <vector>

namespace saauso::internal {

std::vector<void*> temp_addrs;

void* Heap::AllocateRaw(size_t size_in_bytes, AllocationSpace space) {
  void* result = std::malloc(size_in_bytes);
  std::memset(result, 0x00, size_in_bytes);
  temp_addrs.push_back(result);
  return result;
}

void Heap::DoGc() {
  for (auto addr : temp_addrs) {
    std::free(addr);
  }
}

}  // namespace saauso::internal
