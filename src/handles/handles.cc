// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"

#include "include/saauso-internal.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

HandleScope::State HandleScope::current_;

// 保存父级scope的现场
HandleScope::HandleScope() : previous_(current_) {
  current_.extension = 0;  // 当前scope申请的block数要从零开始重新计数
}

HandleScope::~HandleScope() {
  Close();
}

void HandleScope::Close() {
  if (!is_closed_) {
    Universe::handle_scope_implementer_->ReleaseSpareAndExtendedBlocks(
        current_.extension);
    current_ = previous_;  // 恢复父级scope的现场
    is_closed_ = true;
  }
}

Address* HandleScope::EscapeFromSelf(Address ptr) {
  // 释放自己
  assert(!is_closed_);
  Close();
  // 把ptr放置在父级的scope上
  return CreateHandle(ptr);
}

// static
Address* HandleScope::CreateHandle(Address ptr) {
  assert(current_.extension != -1 &&
         "Cannot create a handle without a HandleScope");

  if (current_.next == current_.limit) {
    Extend();
  }

  Address* location = current_.next;
  *location = ptr;
  ++current_.next;
  return location;
}

// static
void HandleScope::Extend() {
  HandleScopeImplementer* impl = Universe::handle_scope_implementer_;
  Address* new_block = impl->AllocateSpareOrNewBlock();
  current_.extension++;
  current_.next = new_block;
  current_.limit = new_block + HandleScopeImplementer::kHandleBlockSize;
}

}  // namespace saauso::internal
