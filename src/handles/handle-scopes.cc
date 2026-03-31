// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "include/saauso-internal.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/handles/handles.h"

namespace saauso::internal {

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
    auto* impl = isolate_->handle_scope_implementer();
    auto* handle_scope_state = impl->handle_scope_state();

    // 释放申请的blocks
    impl->ReleaseSpareAndExtendedBlocks(handle_scope_state->extension);
    // 恢复父级scope的现场
    impl->set_handle_scope_state(&previous_);

#if defined(_DEBUG) || defined(ASAN_BUILD)
    // 恢复父 scope 后，将当前活跃 block 中“父 scope 未使用的尾部区域”做 zap。
    // 这样内层 scope 中未 Escape 的 handle 若仍指向这段区域，
    // 后续解引用时通常能以 fail-fast 形式暴露出来。
    ZapRange(impl->handle_scope_state()->next,
             impl->handle_scope_state()->limit);
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)

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

#if defined(_DEBUG) || defined(ASAN_BUILD)
// static
void HandleScope::ZapRange(Address* start, Address* end) {
  if (start == nullptr) {
    return;
  }

  assert(start <= end);
  assert(end - start <= HandleScopeImplementer::kHandleBlockSize);

  // 这里只负责把“仍然可访问的失效 slot”标成 zap。
  // 对于已经真正释放掉的扩展 block，最终兜底仍依赖 ASAN 的 UAF 检测。
  for (Address* p = start; p < end; p++) {
    *p = kHandleZapValue;
  }
}
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)

}  // namespace saauso::internal
