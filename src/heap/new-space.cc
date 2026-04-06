// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/new-space.h"

#include <algorithm>
#include <cassert>

namespace saauso::internal {

///////////////////////////////////////////////////////////////////////////////
// NewPage 实现开始

Address NewPage::AllocateAndUpdateTop(size_t size_in_bytes) {
  assert(allocation_top_ + size_in_bytes <= allocation_limit_);
  Address result = allocation_top_;
  allocation_top_ += size_in_bytes;
  return result;
}

///////////////////////////////////////////////////////////////////////////////
// NewSpace 实现开始

// static
void NewSpace::InitializePage(NewPage* page,
                              NewSpace* owner,
                              Address page_start,
                              BasePage::Flag flag) {
  PagedSpace::InitializePageHeader(page, owner, page_start, flag,
                                   sizeof(NewPage));
}

// static
NewPage* NewSpace::FindPageWithLinearAllocationArea(NewPage* start_page,
                                                    size_t aligned_size) {
  for (NewPage* page = start_page; page != nullptr; page = page->next()) {
    if (page->allocation_top_ + aligned_size <= page->allocation_limit_) {
      return page;
    }
  }
  return nullptr;
}

void NewSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(IsAddressAlignedToPage(start));

  assert(size >= BasePage::kPageSizeInBytes * 2);
  assert(IsSizeAlignedToPage(size));

  base_ = start;
  end_ = start + size;

  size_t page_count = size / BasePage::kPageSizeInBytes;
  // 页面总数必须是>=2的偶数，便于 eden 和 survivor 空间对半分
  assert(page_count >= 2);
  assert((page_count & 1) == 0);

  size_t semi_page_count = page_count >> 1;
  BasePage* prev = nullptr;

  // 初始化 eden 空间的内存页
  size_t curr_page_count = 0;
  for (; curr_page_count < semi_page_count; ++curr_page_count) {
    Address page_start = base_ + curr_page_count * BasePage::kPageSizeInBytes;
    auto* page = reinterpret_cast<NewPage*>(page_start);
    InitializePage(page, this, page_start, BasePage::Flag::kEdenPage);
    page->prev_ = prev;
    if (prev != nullptr) {
      prev->next_ = page;
    }
    prev = page;
  }

  // 初始化 survivor 空间的内存页
  prev = nullptr;
  for (; curr_page_count < page_count; ++curr_page_count) {
    Address page_start = base_ + curr_page_count * BasePage::kPageSizeInBytes;
    auto* page = reinterpret_cast<NewPage*>(page_start);
    InitializePage(page, this, page_start, BasePage::Flag::kSurvivorPage);
    page->prev_ = prev;
    if (prev != nullptr) {
      prev->next_ = page;
    }
    prev = page;
  }

  // 设置 eden 空间指针
  eden_first_page_ = reinterpret_cast<NewPage*>(base_);
  eden_last_page_ = reinterpret_cast<NewPage*>(
      base_ + (semi_page_count - 1) * BasePage::kPageSizeInBytes);
  eden_current_page_ = eden_first_page_;

  // 设置 survivor 空间指针
  survivor_first_page_ = reinterpret_cast<NewPage*>(
      base_ + semi_page_count * BasePage::kPageSizeInBytes);
  survivor_last_page_ = reinterpret_cast<NewPage*>(
      base_ + (page_count - 1) * BasePage::kPageSizeInBytes);
  survivor_current_page_ = survivor_first_page_;
}

void NewSpace::TearDown() {
  for (NewPage* page = reinterpret_cast<NewPage*>(base_);
       page != nullptr && reinterpret_cast<Address>(page) < end_;
       page = page->next()) {
    PagedSpace::ResetPageHeader(page);
  }
  base_ = kNullAddress;
  end_ = kNullAddress;

  eden_first_page_ = nullptr;
  eden_last_page_ = nullptr;
  eden_current_page_ = nullptr;

  survivor_first_page_ = nullptr;
  survivor_last_page_ = nullptr;
  survivor_current_page_ = nullptr;
}

Address NewSpace::AllocateRaw(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  NewPage* page =
      FindPageWithLinearAllocationArea(eden_current_page_, aligned_size);
  // 发生 OOM，返回 null 以触发 GC
  if (page == nullptr) {
    return kNullAddress;
  }

  eden_current_page_ = page;
  return page->AllocateAndUpdateTop(size_in_bytes);
}

Address NewSpace::AllocateInSurvivorSpace(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  NewPage* page =
      FindPageWithLinearAllocationArea(survivor_current_page_, aligned_size);
  // 发生 OOM，返回 null 以触发 GC
  if (page == nullptr) {
    return kNullAddress;
  }

  survivor_current_page_ = page;
  return page->AllocateAndUpdateTop(aligned_size);
}

bool NewSpace::Contains(Address addr) {
  return ContainsInEdenConsistency(addr) || ContainsInSurvivorConsistency(addr);
}

bool NewSpace::ContainsInEdenConsistency(Address addr) const {
  if (addr < base_ || addr >= end_) {
    return false;
  }
  BasePage* base_page = BasePage::FromAddress(addr);
  if (base_page->magic() != BasePage::kMagicHeader ||
      !base_page->HasFlag(BasePage::Flag::kEdenPage) ||
      base_page->owner() != this) {
    return false;
  }
  return base_page->area_start() <= addr && addr < base_page->allocation_top();
}

bool NewSpace::ContainsInSurvivorConsistency(Address addr) const {
  if (addr < base_ || addr >= end_) {
    return false;
  }
  BasePage* base_page = BasePage::FromAddress(addr);
  if (base_page->magic() != BasePage::kMagicHeader ||
      base_page->owner() != this ||
      !base_page->HasFlag(BasePage::Flag::kSurvivorPage)) {
    return false;
  }
  return base_page->area_start() <= addr && addr < base_page->allocation_top();
}

// static
bool NewSpace::ContainsFast(Address addr) {
  return BasePage::InYoungPage(addr);
}

// static
bool NewSpace::ContainsInEdenFast(Address addr) {
  return BasePage::InEdenPage(addr);
}

// static
bool NewSpace::ContainsInSurvivorFast(Address addr) {
  return BasePage::InSurvivorPage(addr);
}

Address NewSpace::EdenSpaceBase() const {
  assert(eden_first_page_ != nullptr);
  return eden_first_page_->area_start();
}

Address NewSpace::EdenSpaceTop() const {
  assert(eden_current_page_ != nullptr);
  return eden_current_page_->allocation_top();
}

Address NewSpace::SurvivorSpaceBase() const {
  assert(survivor_first_page_ != nullptr);
  return survivor_first_page_->area_start();
}

Address NewSpace::SurvivorSpaceTop() const {
  assert(survivor_current_page_ != nullptr);
  return survivor_current_page_->allocation_top();
}

// static
void NewSpace::SetPagesFlagInSemiSpace(NewPage* first, BasePage::Flag flag) {
  assert(first != nullptr);
  for (NewPage* page = first; page != nullptr; page = page->next()) {
    page->ClearFlag(BasePage::Flag::kEdenPage | BasePage::Flag::kSurvivorPage);
    page->SetFlag(flag);
  }
}

// static
void NewSpace::ResetPageAllocateStatesInSemiSpace(NewPage* first) {
  assert(first != nullptr);
  for (NewPage* page = first; page != nullptr; page = page->next()) {
    // 将内存页的 bump 指针重新拨回分配区头部
    page->allocation_top_ = page->area_start_;
  }
}

void NewSpace::Flip() {
  // 交换 eden 和 survivor 空间的指针
  std::swap(eden_first_page_, survivor_first_page_);
  std::swap(eden_last_page_, survivor_last_page_);

  // 原先的 survivor 空间接下来要被当做 eden 空间使用了，
  // 把它的分配状态（即当前哪个页正在准备继续被分配）转移给 eden 空间。
  eden_current_page_ = survivor_current_page_;

  // 分别重新设置 eden 和 survivor 空间内存页的 flag
  SetPagesFlagInSemiSpace(eden_first_page_, BasePage::Flag::kEdenPage);
  SetPagesFlagInSemiSpace(survivor_first_page_, BasePage::Flag::kSurvivorPage);

  // 清空 survivor 空间的分配状态，以便下一次 GC 时使用
  ResetPageAllocateStatesInSemiSpace(survivor_first_page_);
  survivor_current_page_ = survivor_first_page_;
}

}  // namespace saauso::internal
