// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_OLD_SPACE_H_
#define SAAUSO_HEAP_OLD_SPACE_H_

#include "src/heap/old-page.h"
#include "src/heap/spaces.h"

namespace saauso::internal {

class OldSpace : public PagedSpace {
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
  friend class MarkSweepCollector;
  static OldPage* FindPageWithLinearAllocationArea(OldPage* start_page,
                                                   OldPage* first_page,
                                                   size_t aligned_size);
  static void InitializePage(OldPage* page,
                             OldSpace* owner,
                             Address page_start);

  Address base_{kNullAddress};
  Address end_{kNullAddress};
  OldPage* first_page_{nullptr};
  OldPage* current_page_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_OLD_SPACE_H_
