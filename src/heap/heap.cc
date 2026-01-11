// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "include/saauso-internal.h"
#include "src/handles/handle_scope_implementer.h"
#include "src/heap/scavenge-visitor.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
#include "src/runtime/universe.h"
#include "src/utils/allocation.h"

namespace saauso::internal {

void Heap::Setup(size_t young_generation_size,
                 size_t old_generation_size,
                 size_t meta_space_size) {
  // TODO: 实现大块虚拟内存的灵活生长和收缩
  initial_size_ = young_generation_size + old_generation_size + meta_space_size;
  initial_chunk_ = new VirtualMemory(initial_size_);
  initial_chunk_->Commit(initial_chunk_->address(), initial_size_, false);

  // 初始化各个空间
  auto chunk_start_addr = reinterpret_cast<Address>(initial_chunk_->address());
  new_space_.Setup(chunk_start_addr, young_generation_size * 2);
  chunk_start_addr += young_generation_size * 2;

  old_space_.Setup(chunk_start_addr, old_generation_size);
  chunk_start_addr += old_generation_size;

  meta_space_.Setup(chunk_start_addr, meta_space_size);
  chunk_start_addr += meta_space_size;
}

void Heap::TearDown() {
  initial_chunk_->Uncommit(initial_chunk_->address(), initial_size_);
  delete initial_chunk_;
}

Address Heap::AllocateRaw(size_t size_in_bytes, AllocationSpace space) {
  Address result = AllocateRawImpl(size_in_bytes, space);
  // TODO: 实现对老生代的回收
  if (result == kNullAddress && space == AllocationSpace::kNewSpace) {
    CollectGarbage();
    result = AllocateRawImpl(size_in_bytes, space);
  }

  if (result == kNullAddress) {
    std::printf("OOM: Failed to allocate %zu bytes in space %d\n",
                size_in_bytes, static_cast<int>(space));
    std::exit(1);
  }

  return result;
}

Address Heap::AllocateRawImpl(size_t size_in_bytes, AllocationSpace space) {
  assert(gc_state_ == GcState::kNotInGc &&
         "can't allocate in heap while gc running!!!");

  Address result = kNullAddress;
  switch (space) {
    case AllocationSpace::kNewSpace:
      result = new_space_.AllocateRaw(size_in_bytes);
      break;
    case AllocationSpace::kOldSpace:
      result = old_space_.AllocateRaw(size_in_bytes);
      break;
    case AllocationSpace::kMetaSpace:
      result = meta_space_.AllocateRaw(size_in_bytes);
      break;
    default:
      assert(0 && "unknown heap space!!!");
  }

  return result;
}

bool Heap::InNewSpaceEden(Address addr) {
  return new_space().eden_space().Contains(addr);
}

bool Heap::InNewSpaceSurvivor(Address addr) {
  return new_space().survivor_space().Contains(addr);
}

void Heap::CollectGarbage() {
  gc_state_ = GcState::kScavenage;
  DoScavenge();
  gc_state_ = GcState::kNotInGc;
}

void Heap::IterateRoots(ObjectVisitor* v) {
  // 遍历所有klass，处理klass内部持有引用的对象
  for (auto i = 0; i < Universe::klass_list_.length(); ++i) {
    Universe::klass_list_.Get(i)->Iterate(v);
  }

  // 遍历handle，处理所有handle scope block中持有引用的对象
  Universe::handle_scope_implementer_->Iterate(v);

  // TODO: 遍历python运行时的GC ROOT
}

void Heap::DoScavenge() {
  ScavenageVisitor visitor;

#ifdef _DEBUG
  std::cout << "new_space_.SurvivorSpaceBase() = "
            << new_space_.SurvivorSpaceBase() << std::endl;
  std::cout << "new_space_.SurvivorSpaceTop() = "
            << new_space_.SurvivorSpaceTop() << std::endl;

  std::cout << "start to iterate root" << std::endl;
#endif  //_DEBUG

  // 遍历GC ROOTS，把所有的GC ROOT从eden空间拷贝到survivor空间
  IterateRoots(&visitor);

#ifdef _DEBUG
  std::cout << "finish to iterate root" << std::endl;
#endif  //_DEBUG

  // 此时：
  // new_space_->SurvivorSpaceBase() 指向第一个被拷贝的root对象
  // new_space_->SurvivorSpaceTop() 指向最后一个被拷贝的root对象的末尾
  Address scan_ptr = ObjectSizeAlign(new_space_.SurvivorSpaceBase());
  Address threshold = new_space_.SurvivorSpaceTop();

#ifdef _DEBUG
  std::cout << "new_space_.SurvivorSpaceBase() = "
            << new_space_.SurvivorSpaceBase() << std::endl;
  std::cout << "ObjectSizeAlign(new_space_.SurvivorSpaceBase()) = "
            << ObjectSizeAlign(new_space_.SurvivorSpaceBase()) << std::endl;
  std::cout << "new_space_.SurvivorSpaceTop() = "
            << new_space_.SurvivorSpaceTop() << std::endl;

  std::cout << "start doing bfs" << std::endl;
#endif  //_DEBUG

  // 只要scan_ptr还没追上threshold，说明还有对象没被扫描
  while (scan_ptr < threshold) {
#ifdef _DEBUG
    std::cout << "start visit object " << scan_ptr << std::endl;
#endif  // _DEBUG

    // 获取当前要扫描的对象
    Tagged<PyObject> object(scan_ptr);
    size_t instance_size = PyObject::GetInstanceSize(object);

    // 扫描该对象，将该对象持有引用的子对象全部拷贝到survivor空间
    PyObject::Iterate(object, &visitor);

    // 移动scan_ptr指针，准备扫描下一个对象
    scan_ptr += instance_size;

    // 可能又有新的对象被拷贝到survivor空间了，再次更新threshold
    threshold = new_space_.SurvivorSpaceTop();
#ifdef _DEBUG
    std::cout << "finish visit object " << scan_ptr << std::endl;
    std::cout << "threshold " << threshold << " -> "
              << new_space_.SurvivorSpaceTop() << std::endl;
#endif  // _DEBUG
  }

  // 交换空间
  new_space_.Flip();

  // 重置survivor空间的内部指针
  new_space_.survivor_space().Reset();
}

}  // namespace saauso::internal
