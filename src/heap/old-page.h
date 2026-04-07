// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_OLD_PAGE_H_
#define SAAUSO_HEAP_OLD_PAGE_H_

#include "src/heap/base-page.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class OldSpace;

class OldFreeBlock {
 public:
  static constexpr size_t kMinimumSizeInBytes =
      (sizeof(size_t) + sizeof(OldFreeBlock*) + kObjectAlignmentMask) &
      ~kObjectAlignmentMask;

  size_t size() const { return size_; }
  OldFreeBlock* next() const { return next_; }

 private:
  friend class OldPage;

  size_t size_;
  OldFreeBlock* next_;
};

class OldPage : public BasePage {
 public:
  using RememberedSet = Vector<Address*>;

  enum class Flag : uintptr_t {
    kLargePage = 1u << 8,
    kNeedsSweep = 1u << 9,
    kSwept = 1u << 10,
  };

  static constexpr uint32_t kMagicHeader = BasePage::kMagicHeader;
  static constexpr size_t kSizeInBytes = BasePage::kPageSizeInBytes;

  OldSpace* owner() const { return reinterpret_cast<OldSpace*>(owner_); }
  OldPage* next() const { return reinterpret_cast<OldPage*>(next_); }
  OldPage* prev() const { return reinterpret_cast<OldPage*>(prev_); }
  size_t live_bytes() const { return live_bytes_; }

  RememberedSet* remembered_set() const { return remembered_set_; }
  size_t remembered_set_length() const {
    return remembered_set_ == nullptr ? 0 : remembered_set_->length();
  }
  OldFreeBlock* free_list_head() const { return free_list_head_; }
  size_t GetFreeListLengthSlow() const;
  bool IsValidFreeBlockSlow(Address addr, size_t size_in_bytes) const;

  void AddRememberedSlot(Address* slot);
  void ClearFreeList();
  void AddFreeBlock(Address addr, size_t size_in_bytes);
  Address TryAllocateFromFreeList(size_t size_in_bytes);

 protected:
  size_t live_bytes_;
  RememberedSet* remembered_set_;
  OldFreeBlock* free_list_head_;

 private:
  friend class OldSpace;
  friend class MarkSweepCollector;
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_OLD_PAGE_H_
