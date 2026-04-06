// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_SCAVENGER_COLLECTOR_H_
#define SAAUSO_HEAP_SCAVENGER_COLLECTOR_H_

namespace saauso::internal {

class Heap;

// 用于执行新生代Cheny算法的垃圾回收器
class ScavengerCollector {
 public:
  explicit ScavengerCollector(Heap* heap) : heap_(heap) {};
  ~ScavengerCollector() = default;

  void CollectGarbage();

 private:
  Heap* heap_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_SCAVENGER_COLLECTOR_H_
