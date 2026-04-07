// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/mark-sweep-collector.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "src/heap/heap.h"
#include "src/heap/old-page.h"
#include "src/heap/old-space.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

namespace {

struct SweepFromPredicateContext {
  OldLiveObjectPredicate predicate;
  void* data;
  MarkSweepCollector::LiveObjectVector live_objects;
};

void CollectAllocatedObject(Address addr,
                            size_t size_in_bytes,
                            void* raw_data) {
  auto* context = reinterpret_cast<SweepFromPredicateContext*>(raw_data);
  if (context->predicate(addr, size_in_bytes, context->data)) {
    context->live_objects.PushBack({addr, size_in_bytes});
  }
}

}  // namespace

void MarkSweepCollector::IterateAllocatedObjectsInPage(
    OldPage* page,
    OldAllocatedObjectCallback callback,
    void* data) const {
  assert(page != nullptr);
  assert(callback != nullptr);

  Address cursor = page->area_start_;
  while (cursor < page->allocation_top_) {
    Tagged<PyObject> object(cursor);
    size_t object_size = ObjectSizeAlign(PyObject::GetInstanceSize(object));
    assert(object_size != 0);
    assert(cursor + object_size <= page->allocation_top_);

    callback(cursor, object_size, data);
    cursor += object_size;
  }
}

void MarkSweepCollector::SweepPageFromLiveObjects(
    OldPage* page,
    const LiveObjectVector& live_objects) const {
  assert(page != nullptr);

  page->ClearFreeList();

  Address cursor = page->area_start_;
  size_t live_bytes = 0;
  for (size_t i = 0; i < live_objects.length(); ++i) {
    const OldLiveObjectInfo& live_object = live_objects.Get(i);
    Address live_addr = live_object.addr;
    size_t live_size = ObjectSizeAlign(live_object.size_in_bytes);

    assert(page->area_start_ <= live_addr);
    assert(live_addr + live_size <= page->allocation_top_);
    assert(cursor <= live_addr);

    size_t dead_size = live_addr - cursor;
    if (dead_size >= OldFreeBlock::kMinimumSizeInBytes) {
      page->AddFreeBlock(cursor, dead_size);
    }

    cursor = live_addr + live_size;
    live_bytes += live_size;
  }

  size_t tail_dead_size = page->allocation_top_ - cursor;
  if (tail_dead_size >= OldFreeBlock::kMinimumSizeInBytes) {
    page->AddFreeBlock(cursor, tail_dead_size);
  }

  page->live_bytes_ = live_bytes;
}

void MarkSweepCollector::SweepPageFromPredicate(
    OldPage* page,
    OldLiveObjectPredicate predicate,
    void* data) const {
  assert(page != nullptr);
  assert(predicate != nullptr);

  SweepFromPredicateContext context{predicate, data, LiveObjectVector()};
  IterateAllocatedObjectsInPage(page, CollectAllocatedObject, &context);
  SweepPageFromLiveObjects(page, context.live_objects);
}

OldSpaceSweepStats MarkSweepCollector::SweepOldSpaceFromPredicate(
    OldLiveObjectPredicate predicate,
    void* data) const {
  assert(predicate != nullptr);

  OldSpaceSweepStats stats{0, 0};
  OldSpace* old_space = heap_->old_space();

  OldPage* first_page = old_space->first_page_;
  OldPage* page = first_page;
  do {
    SweepPageFromPredicate(page, predicate, data);
    ++stats.swept_pages;
    stats.live_bytes += page->live_bytes();
    page = page->next();
  } while (page != first_page);

  old_space->current_page_ = old_space->first_page_;
  return stats;
}

void MarkSweepCollector::CollectLiveObjectsForPage(
    OldPage* page,
    const LiveObjectVector& live_objects,
    LiveObjectVector* page_live_objects) {
  assert(page != nullptr);
  assert(page_live_objects != nullptr);

  std::vector<OldLiveObjectInfo> filtered;
  for (size_t i = 0; i < live_objects.length(); ++i) {
    const OldLiveObjectInfo& live_object = live_objects.Get(i);
    if (page->area_start_ <= live_object.addr &&
        live_object.addr < page->allocation_top_) {
      filtered.push_back(live_object);
    }
  }

  std::sort(filtered.begin(), filtered.end(),
            [](const OldLiveObjectInfo& lhs, const OldLiveObjectInfo& rhs) {
              return lhs.addr < rhs.addr;
            });

  for (const OldLiveObjectInfo& live_object : filtered) {
    page_live_objects->PushBack(live_object);
  }
}

OldSpaceSweepStats MarkSweepCollector::SweepOldSpaceFromLiveObjects(
    const LiveObjectVector& live_objects) const {
  OldSpaceSweepStats stats{0, 0};
  OldSpace* old_space = heap_->old_space();

  OldPage* first_page = old_space->first_page_;
  OldPage* page = first_page;
  do {
    LiveObjectVector page_live_objects;
    CollectLiveObjectsForPage(page, live_objects, &page_live_objects);
    SweepPageFromLiveObjects(page, page_live_objects);
    ++stats.swept_pages;
    stats.live_bytes += page->live_bytes();
    page = page->next();
  } while (page != first_page);

  old_space->current_page_ = old_space->first_page_;
  return stats;
}

}  // namespace saauso::internal
