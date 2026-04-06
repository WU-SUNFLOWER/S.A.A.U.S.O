// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/new-space.h"

#include <cassert>

namespace saauso::internal {

void NewSpace::InitializePage(NewPage* page,
                              NewSpace* owner,
                              Address page_start,
                              BasePage::Flag flag) {
  PagedSpace::InitializePageHeader(page, owner, page_start, flag,
                                   sizeof(NewPage));
}

NewPage* NewSpace::FindPageWithLinearAllocationArea(NewPage* start_page,
                                                    BasePage::Flag flag,
                                                    size_t aligned_size) {
  for (NewPage* page = start_page;
       page != nullptr && page->BasePage::HasFlag(flag); page = page->next()) {
    if (page->allocation_top_ + aligned_size <= page->allocation_limit_) {
      return page;
    }
  }
  return nullptr;
}

void NewSpace::ResetPage(NewPage* page) {
  page->allocation_top_ = page->area_start_;
  page->allocated_bytes_ = 0;
}

void NewSpace::ResetPageRange(NewPage* first, NewPage* last) {
  if (first == nullptr || last == nullptr) {
    return;
  }
  for (NewPage* page = first; page != nullptr; page = page->next()) {
    ResetPage(page);
    if (page == last) {
      return;
    }
  }
}

void NewSpace::SetPageRangeFlag(NewPage* first,
                                NewPage* last,
                                BasePage::Flag flag) {
  if (first == nullptr || last == nullptr) {
    return;
  }
  for (NewPage* page = first; page != nullptr; page = page->next()) {
    page->ClearFlag(BasePage::Flag::kEdenPage);
    page->ClearFlag(BasePage::Flag::kSurvivorPage);
    page->SetFlag(flag);
    if (page == last) {
      return;
    }
  }
}

void NewSpace::Setup(Address start, size_t size) {
  assert(start != kNullAddress);
  assert(IsAddressAlignedToPage(start));

  assert(size >= BasePage::kPageSizeInBytes * 2);
  assert(IsSizeAlignedToPage(size));

  base_ = start;
  end_ = start + size;

  size_t page_count = size / BasePage::kPageSizeInBytes;
  assert(page_count >= 2);
  assert((page_count & 1) == 0);

  size_t eden_page_count = page_count >> 1;
  BasePage* prev = nullptr;
  for (size_t i = 0; i < page_count; ++i) {
    Address page_start = base_ + i * BasePage::kPageSizeInBytes;
    auto* page = reinterpret_cast<NewPage*>(page_start);
    BasePage::Flag flag = i < eden_page_count ? BasePage::Flag::kEdenPage
                                              : BasePage::Flag::kSurvivorPage;
    InitializePage(page, this, page_start, flag);
    page->prev_ = prev;
    if (prev != nullptr) {
      prev->next_ = page;
    }
    prev = page;
  }

  eden_first_page_ = reinterpret_cast<NewPage*>(base_);
  eden_last_page_ = reinterpret_cast<NewPage*>(
      base_ + (eden_page_count - 1) * BasePage::kPageSizeInBytes);
  eden_current_page_ = eden_first_page_;

  survivor_first_page_ = reinterpret_cast<NewPage*>(
      base_ + eden_page_count * BasePage::kPageSizeInBytes);
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
  NewPage* page = FindPageWithLinearAllocationArea(
      eden_current_page_, BasePage::Flag::kEdenPage, aligned_size);
  if (page == nullptr) {
    return kNullAddress;
  }

  Address result = page->allocation_top_;
  page->allocation_top_ += aligned_size;
  page->allocated_bytes_ += aligned_size;
  eden_current_page_ = page;
  return result;
}

Address NewSpace::AllocateInSurvivorSpace(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  NewPage* page = FindPageWithLinearAllocationArea(
      survivor_current_page_, BasePage::Flag::kSurvivorPage, aligned_size);
  if (page == nullptr) {
    return kNullAddress;
  }

  Address result = page->allocation_top_;
  page->allocation_top_ += aligned_size;
  page->allocated_bytes_ += aligned_size;
  survivor_current_page_ = page;
  return result;
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
  return eden_first_page_ == nullptr ? kNullAddress
                                     : eden_first_page_->area_start();
}

Address NewSpace::EdenSpaceTop() const {
  return eden_current_page_ == nullptr ? kNullAddress
                                       : eden_current_page_->allocation_top();
}

Address NewSpace::SurvivorSpaceBase() const {
  return survivor_first_page_ == nullptr ? kNullAddress
                                         : survivor_first_page_->area_start();
}

Address NewSpace::SurvivorSpaceTop() const {
  return survivor_current_page_ == nullptr
             ? kNullAddress
             : survivor_current_page_->allocation_top();
}

void NewSpace::ResetSurvivorSpace() {
  ResetPageRange(survivor_first_page_, survivor_last_page_);
  survivor_current_page_ = survivor_first_page_;
}

void NewSpace::Flip() {
  NewPage* old_eden_first = eden_first_page_;
  NewPage* old_eden_last = eden_last_page_;
  NewPage* old_survivor_first = survivor_first_page_;
  NewPage* old_survivor_last = survivor_last_page_;
  NewPage* old_survivor_current = survivor_current_page_;

  eden_first_page_ = old_survivor_first;
  eden_last_page_ = old_survivor_last;
  eden_current_page_ = old_survivor_current;
  survivor_first_page_ = old_eden_first;
  survivor_last_page_ = old_eden_last;

  SetPageRangeFlag(eden_first_page_, eden_last_page_,
                   BasePage::Flag::kEdenPage);
  SetPageRangeFlag(survivor_first_page_, survivor_last_page_,
                   BasePage::Flag::kSurvivorPage);
  ResetSurvivorSpace();
}

}  // namespace saauso::internal
