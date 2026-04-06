// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/base-page.h"

#include <cassert>

namespace saauso::internal {

bool BasePage::HasFlag(Flag flag) const {
  return (flags_ & static_cast<uintptr_t>(flag)) != 0;
}

void BasePage::SetFlag(Flag flag) {
  flags_ |= static_cast<uintptr_t>(flag);
}

void BasePage::ClearFlag(Flag flag) {
  flags_ &= ~static_cast<uintptr_t>(flag);
}

Address BasePage::PageBase(Address addr) {
  return addr & ~(static_cast<Address>(kPageSizeInBytes) - 1);
}

BasePage* BasePage::FromAddress(Address addr) {
  return reinterpret_cast<BasePage*>(PageBase(addr));
}

}  // namespace saauso::internal
