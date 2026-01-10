// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_HEAP_H_
#define SAAUSO_HEAP_HEAP_H_

#include "src/handles/tagged.h"
#include "src/heap/spaces.h"

namespace saauso::internal {

class ObjectVisitor;

class Heap {
 public:
  enum class AllocationSpace { kNewSpace, kOldSpace, kMetaSpace };

  Address AllocateRaw(size_t size_in_bytes, AllocationSpace space);

  template <typename T>
  Tagged<T> Allocate(AllocationSpace space) {
    return Tagged<T>(AllocateRaw(sizeof(T), space));
  }

  bool InNewSpaceEden(Address raw_addr);
  bool InNewSpaceSurvivor(Address raw_addr);

  void CollectGarbage();

  NewSpace& new_space() { return new_space_; }
  OldSpace& old_space() { return old_space_; }
  MetaSpace& meta_space() { return meta_space_; }

 private:
  Address AllocateRawImpl(size_t size_in_bytes, AllocationSpace space);

  void IterateRoots(ObjectVisitor* v);

  void DoScavenge();

  NewSpace new_space_;
  OldSpace old_space_;
  MetaSpace meta_space_;

  enum class GcState { kNotInGc, kScavenage, kMarkCompact };
  GcState gc_state_{GcState::kNotInGc};
};

#define WRITE_BARRIER

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_HEAP_H_
