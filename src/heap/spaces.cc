// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/spaces.h"

#include <cassert>

namespace saauso::internal {

// static
size_t PagedSpace::AlignSizeToPage(size_t size) {
  size_t mask = BasePage::kPageSizeInBytes - 1;
  return (size + mask) & ~mask;
}

// static
Address PagedSpace::AlignAddressToPage(Address addr) {
  Address mask = static_cast<Address>(BasePage::kPageSizeInBytes - 1);
  return (addr + mask) & ~mask;
}

// static
bool PagedSpace::IsSizeAlignedToPage(size_t size) {
  return AlignSizeToPage(size) == size;
}

// static
bool PagedSpace::IsAddressAlignedToPage(Address addr) {
  return AlignAddressToPage(addr) == addr;
}

// static
void PagedSpace::InitializePageHeader(BasePage* page,
                                      PagedSpace* owner,
                                      Address page_start,
                                      BasePage::Flag flag,
                                      size_t header_size) {
  assert(page != nullptr);
  page->magic_ = BasePage::kMagicHeader;
  page->flags_ = static_cast<uintptr_t>(flag);
  page->owner_ = owner;
  page->next_ = nullptr;
  page->prev_ = nullptr;
  page->area_start_ = page_start + ObjectSizeAlign(header_size);
  page->area_end_ = page_start + BasePage::kPageSizeInBytes;
  page->allocation_top_ = page->area_start_;
  page->allocation_limit_ = page->area_end_;
  page->allocated_bytes_ = 0;
}

// static
void PagedSpace::ResetPageHeader(BasePage* page) {
  assert(page != nullptr);
  page->magic_ = 0;
  page->flags_ = 0;
  page->owner_ = nullptr;
  page->next_ = nullptr;
  page->prev_ = nullptr;
  page->area_start_ = kNullAddress;
  page->area_end_ = kNullAddress;
  page->allocation_top_ = kNullAddress;
  page->allocation_limit_ = kNullAddress;
  page->allocated_bytes_ = 0;
}

}  // namespace saauso::internal
