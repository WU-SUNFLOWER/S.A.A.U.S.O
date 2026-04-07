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
                                                      size_t size_in_bytes) {
  assert(start_page != nullptr);
  MetaPage* page = start_page;
  do {
    if (page->allocation_top_ + size_in_bytes <= page->allocation_limit_) {
      return page;
    }
    page = page->next();
  } while (page != start_page);
  return nullptr;
}

void MetaSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(IsAddressAlignedToPage(start));

  assert(size >= BasePage::kPageSizeInBytes);
  assert(IsSizeAlignedToPage(size));

  base_ = AlignAddressToPage(start);
  end_ = base_ + size;

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

  // 链接第一页和最后一页，形成环形链表
  first_page_->prev_ = prev;
  prev->next_ = first_page_;
}

void MetaSpace::TearDown() {
  MetaPage* page = first_page_;
  do {
    MetaPage* next_page = page->next();
    PagedSpace::ResetPageHeader(page);
    page = next_page;
  } while (page != first_page_);

  base_ = kNullAddress;
  end_ = kNullAddress;

  first_page_ = nullptr;
  current_page_ = nullptr;
}

Address MetaSpace::AllocateRaw(size_t size_in_bytes) {
  MetaPage* page = FindPageWithLinearAllocationArea(current_page_, first_page_,
                                                    size_in_bytes);
  // 已经没有更多内存可以分配，返回 null
  if (page == nullptr) {
    return kNullAddress;
  }

  current_page_ = page;
  return page->AllocateAndUpdateTop(size_in_bytes);
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

// static
bool MetaSpace::ContainsFast(Address addr) {
  return BasePage::InMetaPage(addr);
}

}  // namespace saauso::internal
