// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_BASE_PAGE_H_
#define SAAUSO_HEAP_BASE_PAGE_H_

#include "include/saauso-internal.h"

namespace saauso::internal {

class PagedSpace;

class BasePage {
 public:
  enum class Flag : uintptr_t {
    kNoFlags = 0u,
    kEdenPage = 1u << 0,
    kSurvivorPage = 1u << 1,
    kOldPage = 1u << 2,
    kMetaPage = 1u << 3,
  };

  static constexpr uint32_t kMagicHeader = 0x53415553;
  static constexpr size_t kPageSizeInBytes = 256 * 1024;

  uint32_t magic() const { return magic_; }
  PagedSpace* owner() const { return owner_; }

  BasePage* next() const { return next_; }
  BasePage* prev() const { return prev_; }

  Address area_start() const { return area_start_; }
  Address area_end() const { return area_end_; }

  Address allocation_top() const { return allocation_top_; }
  Address allocation_limit() const { return allocation_limit_; }
  size_t allocated_bytes() const { return allocated_bytes_; }

  bool HasFlag(Flag flag) const;
  void SetFlag(Flag flag);
  void ClearFlag(Flag flag);

  static Address PageBase(Address addr);
  static BasePage* FromAddress(Address addr);

 protected:
  friend class MarkSweepCollector;
  friend class MetaSpace;
  friend class NewSpace;
  friend class OldSpace;
  friend class PagedSpace;

  uint32_t magic_{0};
  uintptr_t flags_{0};
  PagedSpace* owner_{nullptr};
  BasePage* next_{nullptr};
  BasePage* prev_{nullptr};
  Address area_start_{kNullAddress};
  Address area_end_{kNullAddress};
  Address allocation_top_{kNullAddress};
  Address allocation_limit_{kNullAddress};
  size_t allocated_bytes_{0};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_BASE_PAGE_H_
