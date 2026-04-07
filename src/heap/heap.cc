// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/heap/mark-sweep-collector.h"
#include "src/heap/meta-space.h"
#include "src/heap/new-space.h"
#include "src/heap/old-space.h"
#include "src/heap/scavenger-collector.h"
#include "src/interpreter/interpreter.h"
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"
#include "src/utils/allocation.h"

namespace saauso::internal {

DisallowHeapAllocation::DisallowHeapAllocation(Isolate* isolate)
    : DisallowHeapAllocation(isolate->heap()) {}

void Heap::IncrementAllocationDisallowedDepth() {
  ++allocation_disallowed_depth_;
}

void Heap::DecrementAllocationDisallowedDepth() {
  assert(allocation_disallowed_depth_ > 0);
  --allocation_disallowed_depth_;
}

void Heap::Setup(size_t young_generation_size,
                 size_t old_generation_size,
                 size_t meta_space_size) {
  // 初始化各个子空间
  SetUpSpaces(young_generation_size, old_generation_size, meta_space_size);

  // 初始化垃圾回收器
  scavenger_collector_ = new ScavengerCollector(this);
  mark_sweep_collector_ = new MarkSweepCollector(this);
}

void Heap::SetUpSpaces(size_t young_generation_size,
                       size_t old_generation_size,
                       size_t meta_space_size) {
  new_space_ = new NewSpace();
  old_space_ = new OldSpace();
  meta_space_ = new MetaSpace();

  size_t aligned_young_generation_size =
      PagedSpace::AlignSizeToPage(young_generation_size);
  size_t aligned_old_generation_size =
      PagedSpace::AlignSizeToPage(old_generation_size);
  size_t aligned_meta_space_size = PagedSpace::AlignSizeToPage(meta_space_size);
  size_t total_young_generation_size = aligned_young_generation_size * 2;

  // 向操作系统申请虚拟内存
  bool commit_result = false;
  Address chunk_start_addr = kNullAddress;

  initial_size_ = total_young_generation_size + aligned_old_generation_size +
                  aligned_meta_space_size;
  initial_chunk_ = new VirtualMemory(initial_size_, BasePage::kPageSizeInBytes);
  if (!initial_chunk_->IsReserved()) {
    goto ret_with_virtual_memory_oom;
  }

  // 目前 S.A.A.U.S.O 不支持堆内存自动生长，
  // 这里直接一次性要求操作系统分配所有刚才申请的虚拟内存！
  commit_result =
      initial_chunk_->Commit(initial_chunk_->address(), initial_size_, false);
  if (!commit_result) {
    goto ret_with_virtual_memory_oom;
  }

  // 初始化新生代空间
  chunk_start_addr = reinterpret_cast<Address>(initial_chunk_->address());
  new_space_->Setup(chunk_start_addr, total_young_generation_size);
  chunk_start_addr += total_young_generation_size;

  // 初始化老生代空间
  old_space_->Setup(chunk_start_addr, aligned_old_generation_size);
  chunk_start_addr += aligned_old_generation_size;

  // 初始化 Meta 空间
  meta_space_->Setup(chunk_start_addr, aligned_meta_space_size);
  chunk_start_addr += aligned_meta_space_size;
  return;

ret_with_virtual_memory_oom:
  std::printf("Virtual Memory OOM!!!");
  std::exit(1);
  return;
}

void Heap::TearDown() {
  delete scavenger_collector_;
  scavenger_collector_ = nullptr;
  delete mark_sweep_collector_;
  mark_sweep_collector_ = nullptr;

  new_space_->TearDown();
  delete new_space_;
  new_space_ = nullptr;

  old_space_->TearDown();
  delete old_space_;
  old_space_ = nullptr;

  meta_space_->TearDown();
  delete meta_space_;
  meta_space_ = nullptr;

  initial_chunk_->Uncommit(initial_chunk_->address(), initial_size_);
  delete initial_chunk_;
  initial_chunk_ = nullptr;
}

Address Heap::AllocateRaw(size_t size_in_bytes, AllocationSpace space) {
  Address result = AllocateRawImpl(size_in_bytes, space);
  // TODO: 实现对老生代的回收
  if (result == kNullAddress && space == AllocationSpace::kNewSpace) {
    CollectGarbage();
    result = AllocateRawImpl(size_in_bytes, space);
  }

  if (result == kNullAddress) {
    std::fprintf(stderr, "OOM: Failed to allocate %zu bytes in space %d\n",
                 size_in_bytes, static_cast<int>(space));
    std::exit(1);
  }

  return result;
}

Address Heap::AllocateRawImpl(size_t size_in_bytes, AllocationSpace space) {
  assert(gc_state_ == GcState::kNotInGc &&
         "can't allocate in heap while gc running!!!");
  assert(IsAllocationAllowed());

  Address result = kNullAddress;
  switch (space) {
    case AllocationSpace::kNewSpace:
      result = new_space_->AllocateRaw(size_in_bytes);
      break;
    case AllocationSpace::kOldSpace:
      result = old_space_->AllocateRaw(size_in_bytes);
      break;
    case AllocationSpace::kMetaSpace:
      result = meta_space_->AllocateRaw(size_in_bytes);
      break;
    default:
      assert(0 && "unknown heap space!!!");
  }

  // 出于性能考虑，开发阶段我们约定手工初始化分配到的内存块，不执行分配函数中预清零的操作！
  // if (result != kNullAddress) {
  //   std::memset(reinterpret_cast<void*>(result), 0, size_in_bytes);
  // }

  return result;
}

// static
bool Heap::InNewSpaceEdenFast(Address addr_in_heap) {
  return NewSpace::ContainsInEdenFast(addr_in_heap);
}

// static
bool Heap::InNewSpaceSurvivorFast(Address addr_in_heap) {
  return NewSpace::ContainsInSurvivorFast(addr_in_heap);
}

// static
bool Heap::InNewSpaceFast(Address addr_in_heap) {
  return NewSpace::ContainsFast(addr_in_heap);
}

// static
bool Heap::InOldSpaceFast(Address addr) {
  return OldSpace::ContainsFast(addr);
}

void Heap::CollectGarbage() {
  gc_state_ = GcState::kScavenage;
  scavenger_collector_->CollectGarbage();
  gc_state_ = GcState::kNotInGc;
}

void Heap::RecordWrite(Tagged<PyObject> object,
                       Address* slot,
                       Tagged<PyObject> value) {
  // 1. 如果写入的是Smi或NULL，忽略
  if (!IsHeapObject(value)) {
    return;
  }

  Address object_addr = object.ptr();
  Address value_addr = value.ptr();

  // 2. 我们只对 Old（object）-> New（value）的边感兴趣。
  //    也就是说，如果 object 不在 OldSpace，或者 value 不在 NewSpace，则忽略！
  if (InNewSpaceFast(object_addr) || !InNewSpaceFast(value_addr)) {
    return;
  }

  // 4. 记录到记忆集 (Old -> New)
  if (InOldSpaceFast(object_addr)) {
    OldSpace::FromAddress(object_addr)->AddRememberedSlot(slot);
    return;
  }

  // 5. 记录到全局记忆集 (Meta -> New 等场景)
  // 简单去重：这里暂不判重，依靠Scavenge时的清理
  remembered_set_.PushBack(reinterpret_cast<Tagged<PyObject>*>(slot));
}

void Heap::IterateRememberedSet(ObjectVisitor* v) {
  for (OldPage* page = old_space_->first_page(); page != nullptr;
       page = page->next()) {
    auto* remembered_set = page->remembered_set();
    if (remembered_set == nullptr) {
      continue;
    }

    size_t new_size = 0;
    for (size_t i = 0; i < remembered_set->length(); ++i) {
      Address* slot = remembered_set->Get(i);
      Tagged<PyObject>* tagged_slot = reinterpret_cast<Tagged<PyObject>*>(slot);
      Tagged<PyObject> object = *tagged_slot;
      assert(!object.is_null());
      assert(!object.IsSmi());
      assert(InNewSpaceFast(object.ptr()));

      v->VisitPointer(tagged_slot);

      Tagged<PyObject> new_object = *tagged_slot;
      // 如果经过处理后，tagged_slot处指向的object仍然在新生代，那么再次回收进记录集
      if (InNewSpaceFast(new_object.ptr())) {
        remembered_set->Set(new_size++, slot);
      }
    }
    remembered_set->Resize(new_size);
  }

  // 使用双指针法原地清理无效的记录
  size_t new_size = 0;
  for (size_t i = 0; i < remembered_set_.length(); ++i) {
    Tagged<PyObject>* slot = remembered_set_.Get(i);
    Tagged<PyObject> object = *slot;

    // 再次检查：如果slot中的对象已经不是NewSpace对象了（可能被重写，或者晋升了）
    // 则移除该记录
    if (!IsHeapObject(object) || !InNewSpaceFast(object.ptr())) {
      continue;
    }

    // 通知GC访问该槽位
    // 注意：v->VisitPointers可能会更新slot中的值（对象移动）
    v->VisitPointers(slot, slot + 1);

    // 访问后再次检查：如果更新后的对象还在NewSpace，保留记录；否则（晋升）移除
    Tagged<PyObject> new_object = *slot;
    if (IsHeapObject(new_object) && InNewSpaceFast(new_object.ptr())) {
      remembered_set_.Set(new_size++, slot);
    }
  }
  remembered_set_.Resize(new_size);
}

void Heap::IterateRoots(ObjectVisitor* v) {
  // 遍历 Isolate 内部直接持有的数据
  isolate_->Iterate(v);

  // 遍历所有klass，处理klass内部持有引用的对象
  for (size_t i = 0; i < isolate_->klass_list().length(); ++i) {
    isolate_->klass_list().Get(i)->Iterate(v);
  }

  // 遍历handle，处理所有handle scope block中持有引用的对象
  if (isolate_->handle_scope_implementer() != nullptr) {
    isolate_->handle_scope_implementer()->Iterate(v);
  }

  // 遍历字节码解释器中持有的引用
  if (isolate_->interpreter() != nullptr) {
    isolate_->interpreter()->Iterate(v);
  }

  // 遍历模块系统中持有的引用（sys.modules/sys.path）
  if (isolate_->module_manager() != nullptr) {
    isolate_->module_manager()->Iterate(v);
  }

  // 遍历 Isolate 级别的异常状态（pending exception）。
  isolate_->exception_state()->Iterate(v);

  // 现阶段string table中所有的字符串都保存在meta space，暂时不需要开放GC!
  // if (isolate_->string_table() != nullptr) {
  // isolate_->string_table()->Iterate(v);
  //}

  // TODO: 当前版本暂时先不实现分代式GC，这行注释请勿开放！
  // 遍历记忆集 (Remembered Set)，处理跨代引用
  // IterateRememberedSet(v);
}

}  // namespace saauso::internal
