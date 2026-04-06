// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_META_SPACE_H_
#define SAAUSO_HEAP_META_SPACE_H_

#include "src/heap/spaces.h"

namespace saauso::internal {

class MetaPage : public BasePage {
 public:
  static constexpr uint32_t kMagicHeader = BasePage::kMagicHeader;
  static constexpr size_t kSizeInBytes = BasePage::kPageSizeInBytes;

  MetaSpace* owner() const { return reinterpret_cast<MetaSpace*>(owner_); }
  MetaPage* next() const { return reinterpret_cast<MetaPage*>(next_); }
  MetaPage* prev() const { return reinterpret_cast<MetaPage*>(prev_); }

 private:
  friend class MetaSpace;
};

class MetaSpace : public PagedSpace {
 public:
  void Setup(Address start, size_t size) override;
  void TearDown() override;

  Address AllocateRaw(size_t size_in_bytes) override;
  bool Contains(Address addr) override;

  MetaPage* first_page() const { return first_page_; }
  MetaPage* current_page() const { return current_page_; }

 private:
  static void InitializePage(MetaPage* page,
                             MetaSpace* owner,
                             Address page_start);
  static MetaPage* FindPageWithLinearAllocationArea(MetaPage* start_page,
                                                    MetaPage* first_page,
                                                    size_t aligned_size);

  MetaPage* first_page_{nullptr};
  MetaPage* current_page_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_META_SPACE_H_
