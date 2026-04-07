// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/old-page.h"

namespace saauso::internal {

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

  if (prev != nullptr && reinterpret_cast<Address>(prev) + prev->size_ ==
                             reinterpret_cast<Address>(block)) {
    prev->size_ += block->size_;
    prev->next_ = block->next_;
    block = prev;
  }

  if (next != nullptr && reinterpret_cast<Address>(block) + block->size_ ==
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

}  // namespace saauso::internal
