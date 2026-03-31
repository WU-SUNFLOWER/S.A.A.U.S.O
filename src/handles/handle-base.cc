// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handle-base.h"

#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"

namespace saauso::internal {

#if defined(_DEBUG) || defined(ASAN_BUILD)
void HandleBase::CheckDereferenceAllowed() const {
  // 快路径：优先检查 slot 是否已被 zap。
  // 这能较低成本地发现“函数返回了未 Escape 的局部 Handle”这类常见错误。
  //
  // 注意：若该 Handle 指向的扩展 block 已在作用域退出时被 delete/free，
  // 那么这里也可能先表现为 ASAN 捕获到的 use-after-free，
  // 而不是稳定命中 kHandleZapValue。遇到此类 UAF 时，
  // 应优先排查最近是否有未逃逸 Handle 被跨 HandleScope 返回或保存。
  if (*location_ == kHandleZapValue) [[unlikely]] {
    std::fprintf(stderr, "Invalid handle: location_ is kHandleZapValue");
    std::exit(1);
  }

#if SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW
  // 慢路径：通过遍历全体有效 block 进行校验，用于疑难问题深挖。
  // 该检查更重，且依赖 Isolate::Current()，但能辅助定位
  // “slot 地址已不在活动 handle blocks 中” 等结构性错误。
  CheckValidLocationSlow();
#endif  // SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW
}
#endif  // defined(_DEBUG) || defined(ASAN_BUILD)

#if SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW
void HandleBase::CheckValidLocationSlow() const {
  Isolate* isolate = Isolate::Current();
  if (location_ == nullptr) {
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

  uintptr_t addr = reinterpret_cast<uintptr_t>(location_);
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
}
#endif  // SAAUSO_ENABLE_CHECK_VALID_LOC_SLOW

}  // namespace saauso::internal
