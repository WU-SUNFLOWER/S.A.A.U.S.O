// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/old-space.h"

namespace saauso::internal {

Address OldSpace::PageBase(Address addr) {
  return BasePage::PageBase(addr);
}

OldPage* OldSpace::FromAddress(Address addr) {
  return reinterpret_cast<OldPage*>(BasePage::FromAddress(addr));
}

OldPage* OldSpace::FindPageWithLinearAllocationArea(OldPage* start_page,
                                                    OldPage* first_page,
                                                    size_t aligned_size) {
  assert(start_page != nullptr);
  OldPage* page = start_page;
  do {
    if (page->allocation_top_ + aligned_size <= page->allocation_limit_) {
      return page;
    }
    page = page->next() != nullptr ? page->next() : first_page;
  } while (page != start_page);
  return nullptr;
}

void OldSpace::InitializePage(OldPage* page,
                              OldSpace* owner,
                              Address page_start) {
  PagedSpace::InitializePageHeader(page, owner, page_start,
                                   BasePage::Flag::kOldPage, sizeof(OldPage));
  page->live_bytes_ = 0;
  page->remembered_set_ = nullptr;
  page->free_list_head_ = nullptr;
}

void OldSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(size >= kPageSizeInBytes);

  base_ = AlignAddressToPage(start);
  end_ = (start + size) & ~(static_cast<Address>(kPageSizeInBytes) - 1);
  assert(base_ < end_);

  first_page_ = reinterpret_cast<OldPage*>(base_);
  current_page_ = first_page_;

  OldPage* prev = nullptr;
  for (Address page_start = base_; page_start < end_;
       page_start += kPageSizeInBytes) {
    auto* page = reinterpret_cast<OldPage*>(page_start);
    InitializePage(page, this, page_start);
    page->prev_ = prev;
    if (prev != nullptr) {
      prev->next_ = page;
    }
    prev = page;
  }
}

void OldSpace::TearDown() {
  for (OldPage* page = first_page_; page != nullptr; page = page->next()) {
    delete page->remembered_set_;
    page->remembered_set_ = nullptr;
    page->free_list_head_ = nullptr;
    page->live_bytes_ = 0;
    PagedSpace::ResetPageHeader(page);
  }
  base_ = kNullAddress;
  end_ = kNullAddress;
  first_page_ = nullptr;
  current_page_ = nullptr;
}

Address OldSpace::AllocateRaw(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  assert(aligned_size <= kMaxRegularHeapObjectSizeInBytes);

  if (current_page_ == nullptr) {
    return kNullAddress;
  }

  OldPage* start_page = current_page_;
  OldPage* page = start_page;
  do {
    Address result = page->TryAllocateFromFreeList(aligned_size);
    if (result != kNullAddress) {
      current_page_ = page;
      return result;
    }
    page = page->next() != nullptr ? page->next() : first_page_;
  } while (page != start_page);

  page = FindPageWithLinearAllocationArea(current_page_, first_page_,
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

bool OldSpace::Contains(Address addr) {
  if (addr < base_ || addr >= end_) {
    return false;
  }

  BasePage* base_page = BasePage::FromAddress(addr);
  if (base_page->magic() != BasePage::kMagicHeader ||
      base_page->owner() != this ||
      !base_page->HasFlag(BasePage::Flag::kOldPage)) {
    return false;
  }
  return base_page->area_start() <= addr && addr < base_page->allocation_top();
}

// static
bool OldSpace::ContainsFast(Address addr) {
  return BasePage::InOldPage(addr);
}

}  // namespace saauso::internal
