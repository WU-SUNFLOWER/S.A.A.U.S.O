// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/spaces.h"

#include <cassert>

#include "include/saauso-internal.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////
// SemiSpace相关实现

Address SemiSpace::AllocateRaw(size_t size_in_bytes) {
  Address result = top_;
  Address new_top = top_ + size_in_bytes;
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

Address NewSpace::AllocateRaw(size_t size_in_bytes) {
  return eden_space_.AllocateRaw(size_in_bytes);
}

void NewSpace::Flip() {
  SemiSpace t = eden_space_;
  eden_space_ = survivor_space_;
  survivor_space_ = t;
}

////////////////////////////////////////////////////////////
// OldSpace相关实现

Address OldSpace::AllocateRaw(size_t size_in_bytes) {
  assert(0);
  return kNullAddress;
}

bool OldSpace::Contains(Address addr) {
  assert(0);
  return false;
}

}  // namespace saauso::internal
