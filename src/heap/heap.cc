// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#include "include/saauso-internal.h"

namespace saauso::internal {

bool Heap::InNewSpaceEden(Address addr) {
  return new_space().eden_space().Contains(addr);
}

bool Heap::InNewSpaceSurvivor(Address addr) {
  return new_space().survivor_space().Contains(addr);
}

Address Heap::AllocateRaw(size_t size_in_bytes, AllocationSpace space) {
  Address result = AllocateRawImpl(size_in_bytes, space);
  if (result == kNullAddress) {
    CollectGarbage();
    result = AllocateRawImpl(size_in_bytes, space);
  }

  if (result == kNullAddress) {
    std::printf("OOM: Failed to allocate %zu bytes\n", size_in_bytes);
    std::exit(1);
  }

  return result;
}

Address Heap::AllocateRawImpl(size_t size_in_bytes, AllocationSpace space) {
  assert(gc_state_ == GcState::kNotInGc &&
         "can't allocate in heap while gc running!!!");

  Address result = kNullAddress;
  switch (space) {
    case AllocationSpace::kNewSpace:
      result = new_space_.AllocateRaw(size_in_bytes);
    case AllocationSpace::kOldSpace:
      result = old_space_.AllocateRaw(size_in_bytes);
    case AllocationSpace::kMetaSpace:
      result = meta_space_.AllocateRaw(size_in_bytes);
    default:
      assert(0 && "unknown heap space!!!");
  }
  return result;
}

void Heap::CollectGarbage() {
  gc_state_ = GcState::kScavenage;
  DoScavenge();
  gc_state_ = GcState::kNotInGc;
}

void Heap::IterateRoots(ObjectVisitor* v) {

}

void Heap::DoScavenge() {
  // 交换空间
  new_space_.Flip();
}

}  // namespace saauso::internal
