// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <vector>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {
class OldSpaceTest : public VmTestBase {};

struct ScanResult {
  std::vector<Address> addresses;
  std::vector<size_t> sizes;
};

void CollectAllocatedObject(Address addr, size_t size_in_bytes, void* data) {
  auto* result = reinterpret_cast<ScanResult*>(data);
  result->addresses.push_back(addr);
  result->sizes.push_back(size_in_bytes);
}

struct LiveAddressSet {
  Address first;
  Address second;
};

bool MatchLiveAddresses(Address addr, size_t, void* data) {
  auto* live = reinterpret_cast<LiveAddressSet*>(data);
  return addr == live->first || addr == live->second;
}

Address AllocateInitializedOldListIterator(Isolate* isolate) {
  Address addr = isolate->heap()->AllocateRaw(sizeof(PyListIterator),
                                              Heap::AllocationSpace::kOldSpace);
  if (addr == kNullAddress) {
    return kNullAddress;
  }

  Tagged<PyListIterator> iterator(addr);
  PyObject::SetKlass(Tagged<PyObject>(iterator),
                     PyListIteratorKlass::GetInstance(isolate));
  PyObject::SetProperties(Tagged<PyObject>(iterator), Tagged<PyDict>::null());
  iterator->set_owner(Tagged<PyList>::null());
  iterator->set_iter_cnt(0);
  return addr;
}
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
  size_t old_size = old_space.end() - old_base;
  old_space.TearDown();
  old_space.Setup(old_base, OldSpace::kPageSizeInBytes);

  OldPage* single_page = old_space.first_page();
  ASSERT_NE(single_page, nullptr);
  ASSERT_EQ(single_page->next(), nullptr);

  size_t step_size = ObjectSizeAlign(sizeof(PyString));
  while (single_page->allocation_top() + step_size <=
         single_page->allocation_limit()) {
    Address filler = old_space.AllocateRaw(step_size);
    ASSERT_NE(filler, kNullAddress);
    EXPECT_EQ(OldSpace::FromAddress(filler), single_page);
  }

  Address next = old_space.AllocateRaw(sizeof(PyString));
  EXPECT_EQ(next, kNullAddress);

  old_space.TearDown();
  old_space.Setup(old_base, old_size);
}

TEST_F(OldSpaceTest, OldSpaceShouldRejectTooLargeRegularObject) {
  HandleScope scope(isolate_);

  EXPECT_DEATH(isolate_->heap()->AllocateRaw(
                   OldSpace::kMaxRegularHeapObjectSizeInBytes + 1,
                   Heap::AllocationSpace::kOldSpace),
               "");
}

TEST_F(OldSpaceTest, OldPageRememberedSetShouldTrackOldToNewSlots) {
  HandleScope scope(isolate_);

  Address host = isolate_->heap()->AllocateRaw(
      sizeof(PyString), Heap::AllocationSpace::kOldSpace);
  ASSERT_NE(host, kNullAddress);

  OldPage* page = OldSpace::FromAddress(host);
  ASSERT_NE(page, nullptr);
  EXPECT_EQ(page->remembered_set_length(), 0u);

  Handle<PyString> young_value = PyString::New(isolate_, "young-value");
  Tagged<PyObject> slot_value = *young_value;

  isolate_->heap()->RecordWrite(Tagged<PyObject>(host),
                                reinterpret_cast<Address*>(&slot_value),
                                *young_value);

  ASSERT_NE(page->remembered_set(), nullptr);
  EXPECT_EQ(page->remembered_set_length(), 1u);
}

TEST_F(OldSpaceTest, OldPageFreeListShouldReuseFreedBlock) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  size_t block_size = ObjectSizeAlign(sizeof(PyString));
  Address block = old_space.AllocateRaw(block_size);
  ASSERT_NE(block, kNullAddress);

  OldPage* page = OldSpace::FromAddress(block);
  ASSERT_NE(page, nullptr);
  page->AddFreeBlock(block, block_size);

  EXPECT_EQ(page->GetFreeListLengthSlow(), 1u);

  Address reused = old_space.AllocateRaw(block_size);
  EXPECT_EQ(reused, block);
  EXPECT_EQ(page->GetFreeListLengthSlow(), 0u);
}

TEST_F(OldSpaceTest, OldPageShouldIterateAllocatedObjectsLinearly) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  Address old_base = old_space.base();
  size_t old_size = old_space.end() - old_base;
  old_space.TearDown();
  old_space.Setup(old_base, OldSpace::kPageSizeInBytes);

  Address first = AllocateInitializedOldListIterator(isolate_);
  Address second = AllocateInitializedOldListIterator(isolate_);
  Address third = AllocateInitializedOldListIterator(isolate_);
  ASSERT_NE(first, kNullAddress);
  ASSERT_NE(second, kNullAddress);
  ASSERT_NE(third, kNullAddress);

  OldPage* page = OldSpace::FromAddress(first);
  ASSERT_NE(page, nullptr);

  ScanResult result;
  page->IterateAllocatedObjects(CollectAllocatedObject, &result);

  ASSERT_EQ(result.addresses.size(), 3u);
  EXPECT_EQ(result.addresses[0], first);
  EXPECT_EQ(result.addresses[1], second);
  EXPECT_EQ(result.addresses[2], third);

  old_space.TearDown();
  old_space.Setup(old_base, old_size);
}

TEST_F(OldSpaceTest, OldPageFreeListShouldSplitLargeBlock) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  size_t block_size = OldFreeBlock::kMinimumSizeInBytes << 1;
  Address block = old_space.AllocateRaw(block_size);
  ASSERT_NE(block, kNullAddress);

  OldPage* page = OldSpace::FromAddress(block);
  ASSERT_NE(page, nullptr);
  page->AddFreeBlock(block, block_size);

  Address reused = old_space.AllocateRaw(OldFreeBlock::kMinimumSizeInBytes);
  EXPECT_EQ(reused, block);
  ASSERT_NE(page->free_list_head(), nullptr);
  EXPECT_EQ(page->GetFreeListLengthSlow(), 1u);
  EXPECT_EQ(page->free_list_head()->size(), OldFreeBlock::kMinimumSizeInBytes);
}

TEST_F(OldSpaceTest, OldPageFreeListShouldMergeAdjacentBlocks) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  size_t block_size = OldFreeBlock::kMinimumSizeInBytes;
  Address first = old_space.AllocateRaw(block_size);
  Address second = old_space.AllocateRaw(block_size);
  ASSERT_NE(first, kNullAddress);
  ASSERT_NE(second, kNullAddress);
  ASSERT_EQ(first + block_size, second);

  OldPage* page = OldSpace::FromAddress(first);
  ASSERT_NE(page, nullptr);

  page->AddFreeBlock(second, block_size);
  page->AddFreeBlock(first, block_size);

  ASSERT_NE(page->free_list_head(), nullptr);
  EXPECT_EQ(page->GetFreeListLengthSlow(), 1u);
  EXPECT_EQ(page->free_list_head()->size(), block_size << 1);
}

TEST_F(OldSpaceTest, OldPageShouldRejectInvalidFreeBlock) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  size_t block_size = ObjectSizeAlign(sizeof(PyString));
  Address block = old_space.AllocateRaw(block_size);
  ASSERT_NE(block, kNullAddress);

  OldPage* page = OldSpace::FromAddress(block);
  ASSERT_NE(page, nullptr);

  EXPECT_TRUE(page->IsValidFreeBlockSlow(block, block_size));
  EXPECT_FALSE(page->IsValidFreeBlockSlow(block + 1, block_size));
  EXPECT_FALSE(page->IsValidFreeBlockSlow(block, OldFreeBlock::kMinimumSizeInBytes - 1));
  page->AddFreeBlock(block, block_size);
  EXPECT_FALSE(page->IsValidFreeBlockSlow(block, block_size));
}

TEST_F(OldSpaceTest, OldPageSweepShouldBuildMergedFreeList) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  Address old_base = old_space.base();
  size_t old_size = old_space.end() - old_base;
  old_space.TearDown();
  old_space.Setup(old_base, OldSpace::kPageSizeInBytes);

  size_t block_size = ObjectSizeAlign(sizeof(PyString));
  Address first = old_space.AllocateRaw(block_size);
  Address second = old_space.AllocateRaw(block_size);
  Address third = old_space.AllocateRaw(block_size);
  ASSERT_NE(first, kNullAddress);
  ASSERT_NE(second, kNullAddress);
  ASSERT_NE(third, kNullAddress);

  OldPage* page = OldSpace::FromAddress(first);
  ASSERT_NE(page, nullptr);

  OldPage::LiveObjectVector live_objects;
  live_objects.PushBack({second, block_size});
  page->SweepAndBuildFreeList(live_objects);

  EXPECT_TRUE(page->HasFlag(OldPage::Flag::kSwept));
  EXPECT_FALSE(page->HasFlag(OldPage::Flag::kNeedsSweep));
  EXPECT_EQ(page->live_bytes(), block_size);
  EXPECT_EQ(page->GetFreeListLengthSlow(), 2u);

  Address reused_front = old_space.AllocateRaw(block_size);
  EXPECT_EQ(reused_front, first);
  Address reused_tail = old_space.AllocateRaw(block_size);
  EXPECT_EQ(reused_tail, third);

  old_space.TearDown();
  old_space.Setup(old_base, old_size);
}

TEST_F(OldSpaceTest, OldPageSweepFromPredicateShouldBuildFreeList) {
  HandleScope scope(isolate_);

  OldSpace& old_space = isolate_->heap()->old_space();
  Address old_base = old_space.base();
  size_t old_size = old_space.end() - old_base;
  old_space.TearDown();
  old_space.Setup(old_base, OldSpace::kPageSizeInBytes);

  size_t block_size = ObjectSizeAlign(sizeof(PyListIterator));
  Address first = AllocateInitializedOldListIterator(isolate_);
  Address second = AllocateInitializedOldListIterator(isolate_);
  Address third = AllocateInitializedOldListIterator(isolate_);
  ASSERT_NE(first, kNullAddress);
  ASSERT_NE(second, kNullAddress);
  ASSERT_NE(third, kNullAddress);

  OldPage* page = OldSpace::FromAddress(first);
  ASSERT_NE(page, nullptr);

  LiveAddressSet live{second, third};
  page->SweepFromPredicate(MatchLiveAddresses, &live);

  EXPECT_TRUE(page->HasFlag(OldPage::Flag::kSwept));
  EXPECT_EQ(page->live_bytes(), block_size << 1);
  EXPECT_EQ(page->GetFreeListLengthSlow(), 1u);
  ASSERT_NE(page->free_list_head(), nullptr);
  EXPECT_EQ(reinterpret_cast<Address>(page->free_list_head()), first);
  EXPECT_EQ(page->free_list_head()->size(), block_size);

  Address reused = old_space.AllocateRaw(block_size);
  EXPECT_EQ(reused, first);

  old_space.TearDown();
  old_space.Setup(old_base, old_size);
}

}  // namespace saauso::internal
