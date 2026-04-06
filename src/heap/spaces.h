// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SPACES_H_
#define SAAUSO_HEAP_SPACES_H_

#include "include/saauso-internal.h"
#include "src/heap/base-page.h"

namespace saauso::internal {

class Space {
 public:
  virtual void Setup(Address start, size_t size) = 0;
  virtual void TearDown() = 0;

  virtual Address AllocateRaw(size_t size_in_bytes) = 0;
  virtual bool Contains(Address addr) = 0;

 protected:
  ~Space() = default;
};

class PagedSpace : public Space {
 public:
  static constexpr size_t kPageSizeInBytes = BasePage::kPageSizeInBytes;

  Address base() const { return base_; }
  Address end() const { return end_; }

  static Address AlignToPage(Address addr);

 protected:
  static void InitializePageHeader(BasePage* page,
                                   PagedSpace* owner,
                                   Address page_start,
                                   BasePage::Flag flag,
                                   size_t header_size);
  static void ResetPageHeader(BasePage* page);

  Address base_{kNullAddress};
  Address end_{kNullAddress};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_SPACES_H_
