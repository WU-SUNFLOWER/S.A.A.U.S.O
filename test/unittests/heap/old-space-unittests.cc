// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {
class OldSpaceTest : public VmTestBase {};
}  // namespace

TEST_F(OldSpaceTest, OldSpaceShouldAllocatePagedObjects) {
  HandleScope scope(isolate_);

  Address addr = isolate_->heap()->AllocateRaw(
      sizeof(PyString), Heap::AllocationSpace::kOldSpace);

  EXPECT_NE(addr, kNullAddress);
  EXPECT_TRUE(isolate_->heap()->InOldSpace(addr));

  OldPage* page = OldSpace::FromAddress(addr);
  ASSERT_NE(page, nullptr);
  EXPECT_EQ(page->magic(), OldPage::kMagicHeader);
  EXPECT_EQ(page->owner(), &isolate_->heap()->old_space());
  EXPECT_LE(page->area_start(), addr);
  EXPECT_LT(addr, page->allocation_top());
}

TEST_F(OldSpaceTest, OldSpaceContainsShouldRejectAddressOutsideAllocatedArea) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  OldPage* page = old_space.first_page();
  ASSERT_NE(page, nullptr);

  EXPECT_FALSE(old_space.Contains(page->area_end() - 1));
}

TEST_F(OldSpaceTest, OldSpaceShouldAdvanceToNextPageWhenCurrentPageIsFull) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  Address old_base = old_space.base();
  old_space.TearDown();
  old_space.Setup(old_base, OldSpace::kPageSizeInBytes);

  OldPage* single_page = old_space.first_page();
  ASSERT_NE(single_page, nullptr);
  ASSERT_EQ(single_page->next(), nullptr);

  size_t kStepSize = ObjectSizeAlign(sizeof(PyString));
  while (single_page->allocation_top() + kStepSize <=
         single_page->allocation_limit()) {
    Address filler = old_space.AllocateRaw(kStepSize);
    ASSERT_NE(filler, kNullAddress);
    EXPECT_EQ(OldSpace::FromAddress(filler), single_page);
  }

  Address next = old_space.AllocateRaw(sizeof(PyString));
  EXPECT_EQ(next, kNullAddress);
}

TEST_F(OldSpaceTest, OldSpaceShouldRejectTooLargeRegularObject) {
  HandleScope scope(isolate_);

  EXPECT_DEATH(isolate_->heap()->AllocateRaw(
                   OldSpace::kMaxRegularHeapObjectSizeInBytes + 1,
                   Heap::AllocationSpace::kOldSpace),
               "");
}

}  // namespace saauso::internal
