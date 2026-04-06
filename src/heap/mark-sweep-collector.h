// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_MARK_SWEEP_COLLECTOR_H_
#define SAAUSO_HEAP_MARK_SWEEP_COLLECTOR_H_

#include "src/heap/spaces.h"

namespace saauso::internal {

class Heap;

struct OldLiveObjectInfo {
  Address addr;
  size_t size_in_bytes;
};

using OldAllocatedObjectCallback = void (*)(Address, size_t, void*);
using OldLiveObjectPredicate = bool (*)(Address, size_t, void*);

struct OldSpaceSweepStats {
  size_t swept_pages;
  size_t live_bytes;
};

class MarkSweepCollector {
 public:
  using LiveObjectVector = Vector<OldLiveObjectInfo>;

  explicit MarkSweepCollector(Heap* heap) : heap_(heap) {}

  void IterateAllocatedObjectsInPage(OldPage* page,
                                     OldAllocatedObjectCallback callback,
                                     void* data) const;
  void SweepPageFromLiveObjects(OldPage* page,
                                const LiveObjectVector& live_objects) const;
  void SweepPageFromPredicate(OldPage* page,
                              OldLiveObjectPredicate predicate,
                              void* data) const;

  OldSpaceSweepStats SweepOldSpaceFromPredicate(OldLiveObjectPredicate predicate,
                                                void* data) const;
  OldSpaceSweepStats SweepOldSpaceFromLiveObjects(
      const LiveObjectVector& live_objects) const;

 private:
  static void CollectLiveObjectsForPage(OldPage* page,
                                        const LiveObjectVector& live_objects,
                                        LiveObjectVector* page_live_objects);

  Heap* heap_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_MARK_SWEEP_COLLECTOR_H_
