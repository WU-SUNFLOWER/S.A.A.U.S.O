// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"

#include <cstdint>

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"

namespace saauso::internal {

// 保存父级scope的现场
HandleScope::HandleScope() : HandleScope(Isolate::Current()) {}

HandleScope::HandleScope(Isolate* isolate) {
  // 将新建的handle scope与当前的isolate绑定
  isolate_ = isolate;
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

Address* HandleScope::CloseAndEscape(Address ptr) {
  // 释放自己
  assert(!is_closed_);
  Close();
  // 把ptr放置在父级的scope上
  return CreateHandle(isolate_, ptr);
}

// static
Address* HandleScope::CreateHandle(Address ptr) {
  return CreateHandle(Isolate::Current(), ptr);
}

// static
Address* HandleScope::CreateHandle(Isolate* isolate, Address ptr) {
  if (isolate == nullptr) {
    std::printf("Cannot create a handle without an isolate");
    std::exit(1);
  }

  auto* impl = isolate->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();

  if (handle_scope_state->extension == -1) {
    std::printf("Cannot create a handle without a HandleScope");
    std::exit(1);
  }

  if (handle_scope_state->next == handle_scope_state->limit) {
    Extend(isolate);
  }

  Address* location = handle_scope_state->next;
  *location = ptr;
  ++handle_scope_state->next;

  return location;
}

// static
void HandleScope::Extend() {
  Extend(Isolate::Current());
}

// static
void HandleScope::Extend(Isolate* isolate) {
  assert(isolate != nullptr);
  auto* impl = isolate->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();
  Address* new_block = impl->AllocateSpareOrNewBlock();
  handle_scope_state->extension++;
  handle_scope_state->next = new_block;
  handle_scope_state->limit =
      new_block + HandleScopeImplementer::kHandleBlockSize;
}

// static
void HandleScope::AssertValidLocation(Address* location) {
  AssertValidLocation(Isolate::Current(), location);
}

// static
void HandleScope::AssertValidLocation(Isolate* isolate, Address* location) {
#if defined(_DEBUG) || defined(ASAN_BUILD)
  if (location == nullptr) {
    std::fprintf(stderr, "Invalid handle: null location");
    std::exit(1);
  }

  if (isolate == nullptr) {
    std::fprintf(stderr, "Invalid handle: missing isolate");
    std::exit(1);
  }

  auto* impl = isolate->handle_scope_implementer();
  if (impl == nullptr) {
    std::fprintf(stderr, "Invalid handle: missing HandleScopeImplementer");
    std::exit(1);
  }

  auto& blocks = impl->blocks();
  if (blocks.IsEmpty()) {
    std::fprintf(stderr, "Invalid handle: no active handle blocks");
    std::exit(1);
  }

  uintptr_t addr = reinterpret_cast<uintptr_t>(location);
  bool in_active_blocks = false;
  Address* last_block = blocks.GetBack();
  for (size_t i = 0; i < blocks.length(); ++i) {
    Address* block = blocks.Get(i);
    uintptr_t start = reinterpret_cast<uintptr_t>(block);
    uintptr_t end =
        start + sizeof(Address) * HandleScopeImplementer::kHandleBlockSize;
    if (addr < start || addr >= end) {
      continue;
    }

    in_active_blocks = true;
    if (block == last_block) {
      uintptr_t next =
          reinterpret_cast<uintptr_t>(impl->handle_scope_state()->next);
      if (addr >= next) {
        std::fprintf(
            stderr,
            "Invalid handle: location is outside the active handle scope");
        std::exit(1);
      }
    }
    break;
  }

  if (!in_active_blocks) {
    std::fprintf(stderr,
                 "Invalid handle: location is not within active handle blocks");
    std::exit(1);
  }
#endif
}

}  // namespace saauso::internal
