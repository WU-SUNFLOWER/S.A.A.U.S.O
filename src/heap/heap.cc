// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/heap.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/handle-scope-implementer.h"
#include "src/heap/mark-sweep-collector.h"
#include "src/heap/scavenger-collector.h"
#include "src/interpreter/interpreter.h"
#include "src/modules/module-manager.h"
#include "src/objects/klass.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"
#include "src/utils/allocation.h"

namespace saauso::internal {

namespace {

size_t AlignSizeToPage(size_t size) {
  size_t mask = BasePage::kPageSizeInBytes - 1;
  return (size + mask) & ~mask;
}

}  // namespace

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
  size_t aligned_young_generation_size = AlignSizeToPage(young_generation_size);
  size_t aligned_old_generation_size = AlignSizeToPage(old_generation_size);
  size_t aligned_meta_space_size = AlignSizeToPage(meta_space_size);
  size_t total_young_generation_size = aligned_young_generation_size * 2;

  initial_size_ = total_young_generation_size + aligned_old_generation_size +
                  aligned_meta_space_size;
  initial_chunk_ = new VirtualMemory(initial_size_, BasePage::kPageSizeInBytes);
  initial_chunk_->Commit(initial_chunk_->address(), initial_size_, false);

  auto chunk_start_addr = reinterpret_cast<Address>(initial_chunk_->address());
  new_space_.Setup(chunk_start_addr, total_young_generation_size);
  chunk_start_addr += total_young_generation_size;

  old_space_.Setup(chunk_start_addr, aligned_old_generation_size);
  chunk_start_addr += aligned_old_generation_size;

  meta_space_.Setup(chunk_start_addr, aligned_meta_space_size);

  scavenger_collector_ = new ScavengerCollector(this);
  mark_sweep_collector_ = new MarkSweepCollector(this);
}

void Heap::TearDown() {
  delete scavenger_collector_;
  scavenger_collector_ = nullptr;
  delete mark_sweep_collector_;
  mark_sweep_collector_ = nullptr;

  new_space_.TearDown();
  old_space_.TearDown();
  meta_space_.TearDown();

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

  // 出于性能考虑，开发阶段我们约定手工初始化分配到的内存块，不执行分配函数中预清零的操作！
  // if (result != kNullAddress) {
  //   std::memset(reinterpret_cast<void*>(result), 0, size_in_bytes);
  // }

  return result;
}

bool Heap::InNewSpaceEden(Address addr) {
  return new_space().ContainsInEden(addr);
}

bool Heap::InNewSpaceSurvivor(Address addr) {
  return new_space().ContainsInSurvivor(addr);
}

bool Heap::InOldSpace(Address addr) {
  return old_space().Contains(addr);
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
  if (value.is_null() || value.IsSmi()) {
    return;
  }

  // 2. 如果value不在NewSpace，忽略（OldSpace/MetaSpace对象不需要Scavenge）
  // 注意：这里假设NewSpace的判断是准确的（包含Eden和Survivor）
  if (!InNewSpaceEden(value.ptr()) && !InNewSpaceSurvivor(value.ptr())) {
    return;
  }

  // 3. 如果host对象本身就在NewSpace，忽略（Scavenge会自动扫描整个NewSpace）
  if (InNewSpaceEden(object.ptr()) || InNewSpaceSurvivor(object.ptr())) {
    return;
  }

  // 4. 记录到记忆集 (Old -> New)
  if (InOldSpace(object.ptr())) {
    OldSpace::FromAddress(object.ptr())->AddRememberedSlot(slot);
    return;
  }

  // 5. 记录到全局记忆集 (Meta -> New 等场景)
  // 简单去重：这里暂不判重，依靠Scavenge时的清理
  remembered_set_.PushBack(reinterpret_cast<Tagged<PyObject>*>(slot));
}

void Heap::IterateRememberedSet(ObjectVisitor* v) {
  for (OldPage* page = old_space_.first_page(); page != nullptr;
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
      assert(InNewSpaceEden(object.ptr()) || InNewSpaceSurvivor(object.ptr()));

      v->VisitPointer(tagged_slot);

      Tagged<PyObject> new_object = *tagged_slot;
      // 如果经过处理后，tagged_slot处指向的object仍然在新生代，那么再次回收进记录集
      if (InNewSpaceEden(new_object.ptr()) ||
          InNewSpaceSurvivor(new_object.ptr())) {
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
    if (object.IsSmi() || object.is_null() ||
        (!InNewSpaceEden(object.ptr()) && !InNewSpaceSurvivor(object.ptr()))) {
      continue;
    }

    // 通知GC访问该槽位
    // 注意：v->VisitPointers可能会更新slot中的值（对象移动）
    v->VisitPointers(slot, slot + 1);

    // 访问后再次检查：如果更新后的对象还在NewSpace，保留记录；否则（晋升）移除
    Tagged<PyObject> new_object = *slot;
    if (!new_object.IsSmi() && !new_object.is_null() &&
        (InNewSpaceEden(new_object.ptr()) ||
         InNewSpaceSurvivor(new_object.ptr()))) {
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
