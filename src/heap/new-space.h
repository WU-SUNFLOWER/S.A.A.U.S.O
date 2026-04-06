// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_NEW_SPACE_H_
#define SAAUSO_HEAP_NEW_SPACE_H_

#include "src/heap/spaces.h"

namespace saauso::internal {

class NewSpace;

class NewPage : public BasePage {
 public:
  static constexpr uint32_t kMagicHeader = BasePage::kMagicHeader;
  static constexpr size_t kSizeInBytes = BasePage::kPageSizeInBytes;

  NewSpace* owner() const { return reinterpret_cast<NewSpace*>(owner_); }
  NewPage* next() const { return reinterpret_cast<NewPage*>(next_); }
  NewPage* prev() const { return reinterpret_cast<NewPage*>(prev_); }

 private:
  friend class NewSpace;
};

class NewSpace : public PagedSpace {
 public:
  NewSpace() = default;

  void Setup(Address start, size_t size) override;
  void TearDown() override;

  Address AllocateRaw(size_t size_in_bytes) override;

  // 针对任意地址，判定它是否属于当前 NewSpace。
  // 因为涉及到读取 NewSpace 实例内部的状态信息，属于慢路径。
  bool Contains(Address addr) override;
  bool ContainsInEdenConsistency(Address addr) const;
  bool ContainsInSurvivorConsistency(Address addr) const;

  // - 在 GC 等热路径上，如果**明确知道一个有效堆上内存页内部地址**，
  //   可直接走 Fast API 进行快速判定。
  // - 在 Debug 模式下，如果传入的 addr 非法（如指向非内存页已分配区域），
  //   则可能会直接触发断言崩溃！
  static bool ContainsFast(Address addr);
  static bool ContainsInEdenFast(Address addr);
  static bool ContainsInSurvivorFast(Address addr);

  Address EdenSpaceBase() const;
  Address EdenSpaceTop() const;

  Address SurvivorSpaceBase() const;
  Address SurvivorSpaceTop() const;

  void Flip();
  void ResetSurvivorSpace();

  Address AllocateInSurvivorSpace(size_t size_in_bytes);

  NewPage* eden_first_page() const { return eden_first_page_; }
  NewPage* eden_current_page() const { return eden_current_page_; }
  NewPage* survivor_first_page() const { return survivor_first_page_; }
  NewPage* survivor_current_page() const { return survivor_current_page_; }

 private:
  static void InitializePage(NewPage* page,
                             NewSpace* owner,
                             Address page_start,
                             BasePage::Flag flag);
  static NewPage* FindPageWithLinearAllocationArea(NewPage* start_page,
                                                   BasePage::Flag flag,
                                                   size_t aligned_size);
  static void ResetPage(NewPage* page);
  static void ResetPageRange(NewPage* first, NewPage* last);
  static void SetPageRangeFlag(NewPage* first,
                               NewPage* last,
                               BasePage::Flag flag);

  NewPage* eden_first_page_{nullptr};
  NewPage* eden_last_page_{nullptr};
  NewPage* eden_current_page_{nullptr};
  NewPage* survivor_first_page_{nullptr};
  NewPage* survivor_last_page_{nullptr};
  NewPage* survivor_current_page_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_NEW_SPACE_H_
