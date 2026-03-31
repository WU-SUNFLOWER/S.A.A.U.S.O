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
void HandleBase::AssertValidLocation() const {
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
#endif

}  // namespace saauso::internal
