// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/old-space.h"

namespace saauso::internal {

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
    page = page->next();
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

void OldSpace::FinalizePage(OldPage* page) {
  delete page->remembered_set_;
  page->remembered_set_ = nullptr;
  page->free_list_head_ = nullptr;
  page->live_bytes_ = 0;
  PagedSpace::ResetPageHeader(page);
}

void OldSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(IsAddressAlignedToPage(start));

  assert(size >= kPageSizeInBytes);
  assert(IsSizeAlignedToPage(size));

  base_ = start;
  end_ = start + size;

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

  first_page_ = reinterpret_cast<OldPage*>(base_);
  current_page_ = first_page_;

  // 链接第一页和最后一页，形成环形链表
  first_page_->prev_ = prev;
  prev->next_ = first_page_;
}

void OldSpace::TearDown() {
  OldPage* page = first_page_;
  do {
    OldPage* next_page = page->next();
    FinalizePage(page);
    page = next_page;
  } while (page != first_page_);

  base_ = kNullAddress;
  end_ = kNullAddress;
  first_page_ = nullptr;
  current_page_ = nullptr;
}

Address OldSpace::AllocateRaw(size_t size_in_bytes) {
  assert(size_in_bytes <= kMaxRegularHeapObjectSizeInBytes);

  if (current_page_ == nullptr) {
    return kNullAddress;
  }

  OldPage* start_page = current_page_;
  OldPage* page = start_page;
  do {
    Address result = page->TryAllocateFromFreeList(size_in_bytes);
    if (result != kNullAddress) {
      current_page_ = page;
      return result;
    }
    page = page->next();
  } while (page != start_page);

  page = FindPageWithLinearAllocationArea(current_page_, first_page_,
                                          size_in_bytes);
  if (page == nullptr) {
    return kNullAddress;
  }

  current_page_ = page;
  return page->AllocateAndUpdateTop(size_in_bytes);
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
