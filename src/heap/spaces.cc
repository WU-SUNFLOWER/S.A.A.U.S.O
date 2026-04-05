// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/spaces.h"

#include <cassert>

#include "include/saauso-internal.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////
// SemiSpace相关实现

void SemiSpace::Setup(Address start, size_t size) {
  base_ = start;
  top_ = start;
  end_ = start + size;
}

void SemiSpace::TearDown() {
  // do nothing.
}

Address SemiSpace::AllocateRaw(size_t size_in_bytes) {
  Address result = top_;
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  Address new_top = top_ + aligned_size;
  if (new_top <= end_) {
    top_ = new_top;
    return result;
  }
  return kNullAddress;  // 此空间发生OOM！！！
}

bool SemiSpace::Contains(Address addr) {
  return base_ <= addr && addr < end_;
}

////////////////////////////////////////////////////////////
// NewSpace相关实现

void NewSpace::Setup(Address start, size_t size) {
  assert((size & 1) == 0 && "the size of new space must be a multiple of two");

  size_t semi_space_size = size >> 1;
  eden_space_.Setup(start, semi_space_size);
  survivor_space_.Setup(start + semi_space_size, semi_space_size);
}

void NewSpace::TearDown() {
  eden_space_.TearDown();
  survivor_space_.TearDown();
}

Address NewSpace::AllocateRaw(size_t size_in_bytes) {
  return eden_space_.AllocateRaw(size_in_bytes);
}

bool NewSpace::Contains(Address addr) {
  return eden_space_.Contains(addr);
}

void NewSpace::Flip() {
  SemiSpace t = eden_space_;
  eden_space_ = survivor_space_;
  survivor_space_ = t;
}

////////////////////////////////////////////////////////////
// OldSpace相关实现

bool OldPage::HasFlag(Flag flag) const {
  return (flags_ & static_cast<uintptr_t>(flag)) != 0;
}

void OldPage::SetFlag(Flag flag) {
  flags_ |= static_cast<uintptr_t>(flag);
}

void OldPage::ClearFlag(Flag flag) {
  flags_ &= ~static_cast<uintptr_t>(flag);
}

void OldPage::AddRememberedSlot(Address* slot) {
  if (remembered_set_ == nullptr) {
    remembered_set_ = new RememberedSet();
  }
  remembered_set_->PushBack(slot);
}

size_t OldPage::GetFreeListLengthSlow() const {
  size_t length = 0;
  for (OldFreeBlock* block = free_list_head_; block != nullptr;
       block = block->next()) {
    ++length;
  }
  return length;
}

bool OldPage::IsValidFreeBlockSlow(Address addr, size_t size_in_bytes) const {
  if (size_in_bytes < OldFreeBlock::kMinimumSizeInBytes) {
    return false;
  }
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  if (aligned_size < OldFreeBlock::kMinimumSizeInBytes) {
    return false;
  }
  if ((addr & kObjectAlignmentMask) != 0) {
    return false;
  }
  if (addr < area_start_ || addr + aligned_size > allocation_top_) {
    return false;
  }

  for (OldFreeBlock* block = free_list_head_; block != nullptr;
       block = block->next()) {
    Address block_addr = reinterpret_cast<Address>(block);
    Address block_end = block_addr + block->size();
    if (!(addr + aligned_size <= block_addr || block_end <= addr)) {
      return false;
    }
  }
  return true;
}

void OldPage::ClearFreeList() {
  free_list_head_ = nullptr;
}

void OldPage::AddFreeBlock(Address addr, size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  assert(IsValidFreeBlockSlow(addr, aligned_size));

  OldFreeBlock* prev = nullptr;
  OldFreeBlock* next = free_list_head_;
  while (next != nullptr && reinterpret_cast<Address>(next) < addr) {
    prev = next;
    next = next->next();
  }

  auto* block = reinterpret_cast<OldFreeBlock*>(addr);
  block->size_ = aligned_size;
  block->next_ = next;

  if (prev == nullptr) {
    free_list_head_ = block;
  } else {
    prev->next_ = block;
  }

  if (prev != nullptr &&
      reinterpret_cast<Address>(prev) + prev->size_ ==
          reinterpret_cast<Address>(block)) {
    prev->size_ += block->size_;
    prev->next_ = block->next_;
    block = prev;
  }

  if (next != nullptr &&
      reinterpret_cast<Address>(block) + block->size_ ==
          reinterpret_cast<Address>(next)) {
    block->size_ += next->size_;
    block->next_ = next->next_;
  }
}

Address OldPage::TryAllocateFromFreeList(size_t size_in_bytes) {
  size_t aligned_size = ObjectSizeAlign(size_in_bytes);
  OldFreeBlock* prev = nullptr;
  OldFreeBlock* block = free_list_head_;

  while (block != nullptr) {
    if (block->size_ >= aligned_size) {
      Address result = reinterpret_cast<Address>(block);
      size_t remainder = block->size_ - aligned_size;
      if (remainder >= OldFreeBlock::kMinimumSizeInBytes) {
        Address next_block_addr = result + aligned_size;
        auto* next_block = reinterpret_cast<OldFreeBlock*>(next_block_addr);
        next_block->size_ = remainder;
        next_block->next_ = block->next_;
        if (prev == nullptr) {
          free_list_head_ = next_block;
        } else {
          prev->next_ = next_block;
        }
      } else if (prev == nullptr) {
        free_list_head_ = block->next_;
      } else {
        prev->next_ = block->next_;
      }
      return result;
    }
    prev = block;
    block = block->next_;
  }

  return kNullAddress;
}

void OldPage::SweepAndBuildFreeList(const LiveObjectVector& live_objects) {
  ClearFlag(Flag::kSwept);
  SetFlag(Flag::kNeedsSweep);
  ClearFreeList();

  Address cursor = area_start_;
  size_t live_bytes = 0;
  for (size_t i = 0; i < live_objects.length(); ++i) {
    const OldLiveObjectInfo& live_object = live_objects.Get(i);
    Address live_addr = live_object.addr;
    size_t live_size = ObjectSizeAlign(live_object.size_in_bytes);

    assert(area_start_ <= live_addr);
    assert(live_addr + live_size <= allocation_top_);
    assert(cursor <= live_addr);

    size_t dead_size = live_addr - cursor;
    if (dead_size >= OldFreeBlock::kMinimumSizeInBytes) {
      AddFreeBlock(cursor, dead_size);
    }

    cursor = live_addr + live_size;
    live_bytes += live_size;
  }

  size_t tail_dead_size = allocation_top_ - cursor;
  if (tail_dead_size >= OldFreeBlock::kMinimumSizeInBytes) {
    AddFreeBlock(cursor, tail_dead_size);
  }

  live_bytes_ = live_bytes;
  ClearFlag(Flag::kNeedsSweep);
  SetFlag(Flag::kSwept);
}

Address OldSpace::PageBase(Address addr) {
  return addr & ~(static_cast<Address>(kPageSizeInBytes) - 1);
}

// static
OldPage* OldSpace::FromAddress(Address addr) {
  return reinterpret_cast<OldPage*>(PageBase(addr));
}

// static
OldPage* OldSpace::FindPageWithLinearAllocationArea(OldPage* start_page,
                                                    OldPage* first_page,
                                                    size_t aligned_size) {
  assert(start_page != nullptr);

  OldPage* page = start_page;
  do {
    if (page->allocation_top_ + aligned_size <= page->allocation_limit_) {
      return page;
    }
    page = page->next_ != nullptr ? page->next_ : first_page;
  } while (page != start_page);

  return nullptr;
}

// static
void OldSpace::InitializePage(OldPage* page,
                              OldSpace* owner,
                              Address page_start) {
  page->magic_ = OldPage::kMagicHeader;
  page->flags_ = static_cast<uintptr_t>(OldPage::Flag::kOldPage);
  page->owner_ = owner;
  page->next_ = nullptr;
  page->prev_ = nullptr;
  page->area_start_ = page_start + ObjectSizeAlign(sizeof(OldPage));
  page->area_end_ = page_start + kPageSizeInBytes;
  page->allocation_top_ = page->area_start_;
  page->allocation_limit_ = page->area_end_;
  page->allocated_bytes_ = 0;
  page->live_bytes_ = 0;
  page->remembered_set_ = nullptr;
  page->free_list_head_ = nullptr;
}

void OldSpace::Setup(Address start, size_t size) {
  assert((kPageSizeInBytes & (kPageSizeInBytes - 1)) == 0);
  assert(start != kNullAddress);
  assert(size >= kPageSizeInBytes);
  Address mask = static_cast<Address>(kPageSizeInBytes - 1);
  Address reserved_end = start + size;
  base_ = (start + mask) & ~mask;
  end_ = reserved_end & ~mask;
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
  for (OldPage* page = first_page_; page != nullptr; page = page->next_) {
    page->magic_ = 0;
    page->flags_ = static_cast<uintptr_t>(OldPage::Flag::kNoFlags);
    page->owner_ = nullptr;
    delete page->remembered_set_;
    page->remembered_set_ = nullptr;
    page->free_list_head_ = nullptr;
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
    page = page->next_ != nullptr ? page->next_ : first_page_;
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

  OldPage* page = FromAddress(addr);
  if (page->magic_ != OldPage::kMagicHeader) {
    return false;
  }
  if (page->owner_ != this) {
    return false;
  }
  return page->area_start_ <= addr && addr < page->allocation_top_;
}

}  // namespace saauso::internal
