// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handle_scope_implementer.h"

#include "include/saauso-internal.h"
#include "src/handles/handles.h"
#include "src/utils/allocation.h"

namespace saauso::internal {

HandleScopeImplementer::~HandleScopeImplementer() {
  while (!blocks_.IsEmpty()) {
    blocks_.PopBack();
  }
  DeleteArray<Address>(spare_);
}

int HandleScopeImplementer::NumberOfHandles() {
  int nr_blocks = blocks().length();
  if (nr_blocks == 0) {
    return 0;
  }

  size_t block_size = HandleScopeImplementer::kHandleBlockSize;
  Address* last_block_base_addr = blocks().GetBack();
  int nr_handles_in_last_block = CurrentHandleScopeState()->next - last_block_base_addr;
  return ((nr_blocks - 1) * block_size) + nr_handles_in_last_block;
}

HandleScope::State* HandleScopeImplementer::CurrentHandleScopeState() const {
  return &HandleScope::current_;
}

Address* HandleScopeImplementer::AllocateSpareOrNewBlock() {
  Address* result = nullptr;
  if (spare_ != nullptr) {
    result = spare_;
    spare_ = nullptr;
  } else {
    result = NewArray<Address>(kHandleBlockSize);
  }
  blocks_.PushBack(result);
  return result;
}

void HandleScopeImplementer::ReleaseSpareAndExtendedBlocks(int n) {
  assert(0 <= n && n <= blocks_.length());

  // 如果当前HandleScope没有申请任何额外的block，
  // 则什么也不做。
  if (n == 0) {
    return;
  }

  for (int i = n; i > 1; --i) {
    DeleteArray<Address>(blocks_.PopBack());
  }

  if (spare_ == nullptr) {
    spare_ = blocks_.PopBack();
  } else {
    DeleteArray<Address>(blocks_.PopBack());
  }
}

}  // namespace saauso::internal
