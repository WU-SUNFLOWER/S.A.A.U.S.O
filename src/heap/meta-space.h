// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_META_SPACE_H_
#define SAAUSO_HEAP_META_SPACE_H_

#include "src/heap/spaces.h"

namespace saauso::internal {

class MetaSpace;

class MetaPage : public BasePage {
 public:
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

  // 调用该方法会严格按照传入的 size_in_bytes 执行分配，
  // 不会做任何内部隐式的大小对齐操作！
  Address AllocateRaw(size_t size_in_bytes) override;

  // 针对任意地址，判定它是否属于当前 NewSpace。
  // 因为涉及到读取 NewSpace 实例内部的状态信息，属于慢路径。
  bool Contains(Address addr) override;

  // - 在 GC 等热路径上，如果**明确知道一个有效堆上内存页内部地址**，
  //   可直接走 Fast API 进行快速判定。
  // - 在 Debug 模式下，如果传入的 addr 非法（如指向非内存页已分配区域），
  //   则可能会直接触发断言崩溃！
  static bool ContainsFast(Address addr);

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
