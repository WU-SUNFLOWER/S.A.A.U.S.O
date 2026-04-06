// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_HEAP_H_
#define SAAUSO_HEAP_HEAP_H_

#include "src/handles/tagged.h"
#include "src/heap/meta-space.h"
#include "src/heap/new-space.h"
#include "src/heap/old-space.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Isolate;
class MarkSweepCollector;
class ObjectVisitor;
class VirtualMemory;

class Heap {
 public:
  enum class AllocationSpace { kNewSpace, kOldSpace, kMetaSpace };

  static constexpr size_t kDefaultYoungGenerationSize = 1 * 1024 * 1024;
  static constexpr size_t kDefaultOldGenerationSize = 5 * 1024 * 1024;
  static constexpr size_t kDefaultMetaSpaceSize = 5 * 1024 * 1024;

  explicit Heap(Isolate* isolate) : isolate_(isolate) {}
  Heap() = delete;

  void Setup(size_t young_generation_size = kDefaultYoungGenerationSize,
             size_t old_generation_size = kDefaultOldGenerationSize,
             size_t meta_space_size = kDefaultMetaSpaceSize);
  void TearDown();

  Address AllocateRaw(size_t size_in_bytes, AllocationSpace space);

  template <typename T>
  Tagged<T> Allocate(AllocationSpace space) {
    return Tagged<T>(AllocateRaw(sizeof(T), space));
  }

  static bool InNewSpaceFast(Address addr_in_heap);
  static bool InNewSpaceEdenFast(Address addr_in_heap);
  static bool InNewSpaceSurvivorFast(Address addr_in_heap);
  static bool InOldSpaceFast(Address addr);

  void CollectGarbage();

  NewSpace& new_space() { return new_space_; }
  OldSpace& old_space() { return old_space_; }
  MetaSpace& meta_space() { return meta_space_; }
  MarkSweepCollector& mark_sweep_collector() { return *mark_sweep_collector_; }

  void IncrementAllocationDisallowedDepth();
  void DecrementAllocationDisallowedDepth();
  bool IsAllocationAllowed() const { return allocation_disallowed_depth_ == 0; }

  // 写屏障入口
  // object: 发生写入的对象（宿主）
  // slot: 写入发生的内存地址（成员变量地址）
  // value: 被写入的值（目标对象）
  void RecordWrite(Tagged<PyObject> object,
                   Address* slot,
                   Tagged<PyObject> value);

  Isolate* isolate() const { return isolate_; }

 private:
  friend class ScavengerCollector;
  friend class MarkSweepCollector;

  enum class GcState { kNotInGc, kScavenage, kMarkCompact };

  Address AllocateRawImpl(size_t size_in_bytes, AllocationSpace space);

  void IterateRoots(ObjectVisitor* v);
  void IterateRememberedSet(ObjectVisitor* v);

  NewSpace new_space_;
  OldSpace old_space_;
  MetaSpace meta_space_;

  // 记忆集：记录从老生代指向新生代的指针
  // 简单实现：使用std::vector作为Sequential Store Buffer
  // 优化TODO：改用Card Table以减少内存开销和重排开销
  Vector<Tagged<PyObject>*> remembered_set_;

  GcState gc_state_{GcState::kNotInGc};

  // TODO: 实现大块虚拟内存的灵活生长和收缩
  size_t initial_size_;
  VirtualMemory* initial_chunk_;

  ScavengerCollector* scavenger_collector_{nullptr};
  MarkSweepCollector* mark_sweep_collector_{nullptr};

  Isolate* isolate_{nullptr};
  int allocation_disallowed_depth_{0};
};

class DisallowHeapAllocation {
 public:
  explicit DisallowHeapAllocation(Heap* heap) : heap_(heap) {
    heap_->IncrementAllocationDisallowedDepth();
  }

  explicit DisallowHeapAllocation(Isolate* isolate);

  DisallowHeapAllocation(const DisallowHeapAllocation&) = delete;
  DisallowHeapAllocation& operator=(const DisallowHeapAllocation&) = delete;

  ~DisallowHeapAllocation() { heap_->DecrementAllocationDisallowedDepth(); }

 private:
  Heap* heap_{nullptr};
};

// TODO: 当前版本暂时先不实现分代式GC，这行注释请勿开放！
// #define WRITE_BARRIER(object, slot_address, value)                         \
//   do {                                                                     \
//     if (Isolate::Current() != nullptr) {                                   \
//       Isolate::Current()->heap()->RecordWrite(                             \
//           object, reinterpret_cast<Address*>(slot_address), value);        \
//     }                                                                      \
//   } while (false)

#define WRITE_BARRIER(object, slot_address, value)

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_HEAP_H_
