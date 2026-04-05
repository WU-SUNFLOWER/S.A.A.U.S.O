// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SPACES_H_
#define SAAUSO_HEAP_SPACES_H_

#include "include/saauso-internal.h"

namespace saauso::internal {

class Space {
 public:
  virtual void Setup(Address start, size_t size) = 0;
  virtual void TearDown() = 0;

  virtual Address AllocateRaw(size_t size_in_bytes) = 0;
  virtual bool Contains(Address) = 0;
};

class SemiSpace : public Space {
 public:
  SemiSpace() = default;

  void Setup(Address start, size_t size) override;
  void TearDown() override;

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

  Address base() { return base_; }
  Address top() { return top_; }
  Address end() { return end_; }

  void Reset() { top_ = base_; }

 private:
  Address base_;
  Address top_;
  Address end_;
};

class NewSpace : public Space {
 public:
  NewSpace() = default;

  void Setup(Address start, size_t size) override;
  void TearDown() override;

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

  Address EdenSpaceBase() { return eden_space_.base(); }
  Address EdenSpaceTop() { return eden_space_.top(); }

  Address SurvivorSpaceBase() { return survivor_space_.base(); }
  Address SurvivorSpaceTop() { return survivor_space_.top(); }

  void Flip();

  SemiSpace& eden_space() { return eden_space_; }
  SemiSpace& survivor_space() { return survivor_space_; }

 private:
  SemiSpace eden_space_;
  SemiSpace survivor_space_;
};

class MetaSpace : public SemiSpace {};

class OldSpace;

class OldPage {
 public:
  enum class Flag : uintptr_t {
    kNoFlags = 0u,
    kOldPage = 1u << 0,
    kLargePage = 1u << 1,
    kPointersFromHereAreInteresting = 1u << 2,
    kPointersToHereAreInteresting = 1u << 3,
    kNeedsSweep = 1u << 4,
    kSwept = 1u << 5,
  };

  static constexpr uint32_t kMagicHeader = 0x534F4C44;
  static constexpr size_t kSizeInBytes = 256 * 1024;

  uint32_t magic() const { return magic_; }
  OldSpace* owner() const { return owner_; }

  OldPage* next() const { return next_; }
  OldPage* prev() const { return prev_; }

  Address area_start() const { return area_start_; }
  Address area_end() const { return area_end_; }

  Address allocation_top() const { return allocation_top_; }
  Address allocation_limit() const { return allocation_limit_; }

 protected:
  uint32_t magic_;
  uintptr_t flags_;
  OldSpace* owner_;
  OldPage* next_;
  OldPage* prev_;
  Address area_start_;
  Address area_end_;
  Address allocation_top_;
  Address allocation_limit_;
  size_t allocated_bytes_;
  size_t live_bytes_;
  void* remembered_set_;
  void* free_list_head_;

 private:
  friend class OldSpace;
};

// TODO: 实现分代式GC
class OldSpace : public Space {
 public:
  static constexpr size_t kPageSizeInBytes = OldPage::kSizeInBytes;
  static constexpr size_t kMaxRegularHeapObjectSizeInBytes =
      (kPageSizeInBytes - sizeof(OldPage)) >> 1;

  void Setup(Address start, size_t size) override;
  void TearDown() override;

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

  static Address PageBase(Address addr);
  static OldPage* FromAddress(Address addr);

  Address base() const { return base_; }
  Address end() const { return end_; }
  OldPage* first_page() const { return first_page_; }
  OldPage* current_page() const { return current_page_; }

 private:
  static void InitializePage(OldPage* page,
                             OldSpace* owner,
                             Address page_start);

  Address base_{kNullAddress};
  Address end_{kNullAddress};
  OldPage* first_page_{nullptr};
  OldPage* current_page_{nullptr};
};

}  // namespace saauso::internal

#endif
