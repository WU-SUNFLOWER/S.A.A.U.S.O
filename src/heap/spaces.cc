// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/spaces.h"

#include <cassert>

#include "include/saauso-internal.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////
// SemiSpace相关实现

void SemiSpace::Setup(Address start, size_t size) {
  base_ = start;
  top_ = start;
  end_ = start + size;
}

void SemiSpace::TearDown() {
  // do nothing.
}

Address SemiSpace::AllocateRaw(size_t size_in_bytes) {
  Address result = top_;
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  Address new_top = top_ + aligned_size;
  if (new_top <= end_) {
    top_ = new_top;
    return result;
  }
  return kNullAddress;  // 此空间发生OOM！！！
}

bool SemiSpace::Contains(Address addr) {
  return base_ <= addr && addr < end_;
}

////////////////////////////////////////////////////////////
// NewSpace相关实现

void NewSpace::Setup(Address start, size_t size) {
  assert((size & 1) == 0 && "the size of new space must be a multiple of two");

  size_t semi_space_size = size >> 1;
  eden_space_.Setup(start, semi_space_size);
  survivor_space_.Setup(start + semi_space_size, semi_space_size);
}

void NewSpace::TearDown() {
  eden_space_.TearDown();
  survivor_space_.TearDown();
}

Address NewSpace::AllocateRaw(size_t size_in_bytes) {
  return eden_space_.AllocateRaw(size_in_bytes);
}

bool NewSpace::Contains(Address addr) {
  return eden_space_.Contains(addr);
}

void NewSpace::Flip() {
  SemiSpace t = eden_space_;
  eden_space_ = survivor_space_;
  survivor_space_ = t;
}

////////////////////////////////////////////////////////////
// OldSpace相关实现

void OldSpace::Setup(Address start, size_t size) {}
void OldSpace::TearDown() {}

Address OldSpace::AllocateRaw(size_t size_in_bytes) {
  assert(0);
  return kNullAddress;
}

bool OldSpace::Contains(Address addr) {
  assert(0);
  return false;
}

}  // namespace saauso::internal
