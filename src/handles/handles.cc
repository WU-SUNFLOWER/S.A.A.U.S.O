// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"

#include "include/saauso-internal.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

// 保存父级scope的现场
HandleScope::HandleScope() {
  // 将新建的handle scope与当前的isolate绑定
  isolate_ = Isolate::Current();
  assert(isolate_ != nullptr);

  auto* impl = isolate_->handle_scope_implementer();
  assert(impl != nullptr);
  assert(impl->isolate() == isolate_);

  // 保存保存父级scope的现场
  auto* handle_scope_state = impl->handle_scope_state();
  previous_ = *handle_scope_state;

  // 当前scope申请的block数要从零开始重新计数
  handle_scope_state->extension = 0;
}

HandleScope::~HandleScope() {
  Close();
}

void HandleScope::Close() {
  if (!is_closed_) {
    // 禁止跨线程释放handle scope
    if (isolate_ != Isolate::Current()) {
      std::printf(
          "Can't release a HandleScope in a thread different with the thread "
          "which created it!!!");
      std::exit(1);
    }

    auto* impl = isolate_->handle_scope_implementer();
    auto* handle_scope_state = impl->handle_scope_state();

    // 释放申请的blocks
    impl->ReleaseSpareAndExtendedBlocks(handle_scope_state->extension);
    // 恢复父级scope的现场
    impl->set_handle_scope_state(&previous_);
    // 更新handle scope标志位，避免重复执行释放操作
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
  auto* impl = Isolate::Current()->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();

  if (handle_scope_state->extension == -1) {
    std::printf("Cannot create a handle without a HandleScope");
    std::exit(1);
  }

  if (handle_scope_state->next == handle_scope_state->limit) {
    Extend();
  }

  Address* location = handle_scope_state->next;
  *location = ptr;
  ++handle_scope_state->next;

  return location;
}

// static
void HandleScope::Extend() {
  auto* impl = Isolate::Current()->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();
  Address* new_block = impl->AllocateSpareOrNewBlock();
  handle_scope_state->extension++;
  handle_scope_state->next = new_block;
  handle_scope_state->limit =
      new_block + HandleScopeImplementer::kHandleBlockSize;
}

}  // namespace saauso::internal
