// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HEAP_HEAP_H_
#define SAAUSO_HEAP_HEAP_H_

#include "src/handles/tagged.h"
#include "src/heap/spaces.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class ObjectVisitor;
class VirtualMemory;

class Heap {
 public:
  static constexpr size_t kDefaultYoungGenerationSize = 1 * 1024 * 1024;
  static constexpr size_t kDefaultOldGenerationSize = 5 * 1024 * 1024;
  static constexpr size_t kDefaultMetaSpaceSize = 5 * 1024 * 1024;

  enum class AllocationSpace { kNewSpace, kOldSpace, kMetaSpace };

  void Setup(size_t young_generation_size = kDefaultYoungGenerationSize,
             size_t old_generation_size = kDefaultOldGenerationSize,
             size_t meta_space_size = kDefaultMetaSpaceSize);
  void TearDown();

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

  // 写屏障入口
  // object: 发生写入的对象（宿主）
  // slot: 写入发生的内存地址（成员变量地址）
  // value: 被写入的值（目标对象）
  void RecordWrite(Tagged<PyObject> object,
                   Address* slot,
                   Tagged<PyObject> value);

 private:
  Address AllocateRawImpl(size_t size_in_bytes, AllocationSpace space);

  void IterateRoots(ObjectVisitor* v);
  void IterateRememberedSet(ObjectVisitor* v);

  void DoScavenge();

  NewSpace new_space_;
  OldSpace old_space_;
  MetaSpace meta_space_;

  // 记忆集：记录从老生代指向新生代的指针
  // 简单实现：使用std::vector作为Sequential Store Buffer
  // 优化TODO：改用Card Table以减少内存开销和重排开销
  Vector<Tagged<PyObject>*> remembered_set_;

  enum class GcState { kNotInGc, kScavenage, kMarkCompact };
  GcState gc_state_{GcState::kNotInGc};

  // TODO: 实现大块虚拟内存的灵活生长和收缩
  size_t initial_size_;
  VirtualMemory* initial_chunk_;
};

// TODO: 实现分代式垃圾回收
// #define WRITE_BARRIER(object, slot_address, value) \
//   Universe::heap_->RecordWrite(                    \
//       object, reinterpret_cast<Address*>(slot_address), value)

#define WRITE_BARRIER(object, slot_address, value)

}  // namespace saauso::internal

#endif  // SAAUSO_HEAP_HEAP_H_
