// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SCAVENAGE_VISITOR_H_
#define SAAUSO_HEAP_SCAVENAGE_VISITOR_H_

#include "src/objects/visitors.h"

namespace saauso::internal {

class Heap;

class ScavenageVisitor : public ObjectVisitor {
 public:
  ScavenageVisitor(Heap* heap);

  void VisitPointers(Tagged<PyObject>* start, Tagged<PyObject>* end) override;
  void VisitKlass(Tagged<Klass>* p) override;

 private:
  bool CanEvacuate(Tagged<PyObject> object);

  void EvacuateObject(Tagged<PyObject>* slot_ptr);

  Address AllocateInSurvivorSpace(size_t size);

  Heap* heap_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_SCAVENAGE_VISITOR_H_
