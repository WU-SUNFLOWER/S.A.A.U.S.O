// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/meta-space.h"

#include <cassert>

namespace saauso::internal {

void MetaSpace::InitializePage(MetaPage* page,
                               MetaSpace* owner,
                               Address page_start) {
  PagedSpace::InitializePageHeader(page, owner, page_start,
                                   BasePage::Flag::kMetaPage, sizeof(MetaPage));
}

MetaPage* MetaSpace::FindPageWithLinearAllocationArea(MetaPage* start_page,
                                                      MetaPage* first_page,
                                                      size_t aligned_size) {
  assert(start_page != nullptr);
  MetaPage* page = start_page;
  do {
    if (page->allocation_top_ + aligned_size <= page->allocation_limit_) {
      return page;
    }
    page = page->next() != nullptr ? page->next() : first_page;
  } while (page != start_page);
  return nullptr;
}

void MetaSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(size >= BasePage::kPageSizeInBytes);

  base_ = AlignAddressToPage(start);
  end_ =
      (start + size) & ~(static_cast<Address>(BasePage::kPageSizeInBytes) - 1);
  assert(base_ < end_);

  MetaPage* prev = nullptr;
  for (Address page_start = base_; page_start < end_;
       page_start += BasePage::kPageSizeInBytes) {
    auto* page = reinterpret_cast<MetaPage*>(page_start);
    InitializePage(page, this, page_start);
    page->prev_ = prev;
    if (prev != nullptr) {
      prev->next_ = page;
    }
    prev = page;
  }
  first_page_ = reinterpret_cast<MetaPage*>(base_);
  current_page_ = first_page_;
}

void MetaSpace::TearDown() {
  for (MetaPage* page = first_page_; page != nullptr; page = page->next()) {
    PagedSpace::ResetPageHeader(page);
  }
  base_ = kNullAddress;
  end_ = kNullAddress;
  first_page_ = nullptr;
  current_page_ = nullptr;
}

Address MetaSpace::AllocateRaw(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  if (current_page_ == nullptr) {
    return kNullAddress;
  }

  MetaPage* page = FindPageWithLinearAllocationArea(current_page_, first_page_,
                                                    aligned_size);
  if (page == nullptr) {
    return kNullAddress;
  }

  Address result = page->allocation_top_;
  page->allocation_top_ += aligned_size;
  page->allocated_bytes_ += aligned_size;
  current_page_ = page;
  return result;
}

bool MetaSpace::Contains(Address addr) {
  if (addr < base_ || addr >= end_) {
    return false;
  }
  BasePage* base_page = BasePage::FromAddress(addr);
  if (base_page->magic() != BasePage::kMagicHeader ||
      base_page->owner() != this ||
      !base_page->HasFlag(BasePage::Flag::kMetaPage)) {
    return false;
  }
  return base_page->area_start() <= addr && addr < base_page->allocation_top();
}

}  // namespace saauso::internal
