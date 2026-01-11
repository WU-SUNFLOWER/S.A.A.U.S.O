// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SCAVENAGE_VISITOR_H_
#define SAAUSO_HEAP_SCAVENAGE_VISITOR_H_

#include "src/objects/visitors.h"

namespace saauso::internal {

class ScavenageVisitor : public ObjectVisitor {
 public:
  void VisitPointers(Tagged<PyObject>* start, Tagged<PyObject>* end) override;
  void VisitKlass(Tagged<Klass>* p) override;

 private:
  void EvacuateObject(Tagged<PyObject>* slot_ptr);
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_SCAVENAGE_VISITOR_H_
