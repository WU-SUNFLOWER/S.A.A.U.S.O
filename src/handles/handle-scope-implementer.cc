// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handle-scope-implementer.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/visitors.h"
#include "src/utils/allocation.h"

namespace saauso::internal {

HandleScopeImplementer::HandleScopeImplementer(Isolate* isolate)
    : isolate_(isolate) {}

HandleScopeImplementer::~HandleScopeImplementer() {
  while (!blocks_.IsEmpty()) {
    DeleteArray<Address>(blocks_.PopBack());
  }
  DeleteArray<Address>(spare_);

  while (!global_blocks_.IsEmpty()) {
    DeleteArray<Address>(global_blocks_.PopBack());
  }
}

int HandleScopeImplementer::NumberOfHandles() {
  int nr_blocks = blocks().length();
  if (nr_blocks == 0) {
    return 0;
  }

  size_t block_size = HandleScopeImplementer::kHandleBlockSize;
  Address* last_block_base_addr = blocks().GetBack();
  int nr_handles_in_last_block =
      handle_scope_state()->next - last_block_base_addr;
  return ((nr_blocks - 1) * block_size) + nr_handles_in_last_block;
}

HandleScope::State* HandleScopeImplementer::handle_scope_state() {
  return &handle_scope_state_;
}

void HandleScopeImplementer::set_handle_scope_state(HandleScope::State* state) {
  handle_scope_state_ = *state;
}

Address* HandleScopeImplementer::AllocateSpareOrNewBlock() {
  Address* result = nullptr;
  if (spare_ != nullptr) {
    result = spare_;
    spare_ = nullptr;
  } else {
    result = NewArray<Address>(kHandleBlockSize);
  }
  blocks_.PushBack(result);
  return result;
}

void HandleScopeImplementer::ReleaseSpareAndExtendedBlocks(int n) {
  assert(n >= 0 && static_cast<size_t>(n) <= blocks_.length());

  // 如果当前HandleScope没有申请任何额外的block，
  // 则什么也不做。
  if (n == 0) {
    return;
  }

  for (int i = n; i > 1; --i) {
    DeleteArray<Address>(blocks_.PopBack());
  }

  if (spare_ == nullptr) {
    spare_ = blocks_.PopBack();
  } else {
    DeleteArray<Address>(blocks_.PopBack());
  }
}

Address* HandleScopeImplementer::CreateGlobalHandle(Address ptr) {
  Address* location = nullptr;
  if (!global_free_slot_list_.IsEmpty()) {
    location = global_free_slot_list_.PopBack();
  } else {
    if (global_next_ == global_limit_) {
      Address* new_block = NewArray<Address>(kHandleBlockSize);
      global_blocks_.PushBack(new_block);
      global_next_ = new_block;
      global_limit_ = new_block + kHandleBlockSize;
    }
    location = global_next_;
    ++global_next_;
  }

  *location = ptr;
  ++global_handle_count_;
  return location;
}

void HandleScopeImplementer::DestroyGlobalHandle(Address* location) {
  assert(location != nullptr);
  *location = kNullAddress;                   // 置空，避免GC扫描时候意外访问到
  global_free_slot_list_.PushBack(location);  // 回收当前global handle释放的槽位
  --global_handle_count_;
}

void HandleScopeImplementer::Iterate(ObjectVisitor* v) {
  // 遍历所有填充完整的block
  if (blocks_.length() > 1) {
    for (size_t i = 0; i < blocks_.length() - 1; ++i) {
      Address* block_begin = blocks_.Get(i);
      Address* block_end = block_begin + kHandleBlockSize;
      v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                       reinterpret_cast<Tagged<PyObject>*>(block_end));
    }
  }

  // 遍历最后一个block
  if (!blocks_.IsEmpty()) {
    Address* block_begin = blocks_.GetBack();
    Address* block_end = handle_scope_state_.next;
    v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                     reinterpret_cast<Tagged<PyObject>*>(block_end));
  }

  // spare_指向的block没有被分配出来，因此不需要遍历！！！

  // 遍历global blocks
  if (global_blocks_.length() > 1) {
    for (size_t i = 0; i < global_blocks_.length() - 1; ++i) {
      Address* block_begin = global_blocks_.Get(i);
      Address* block_end = block_begin + kHandleBlockSize;
      v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                       reinterpret_cast<Tagged<PyObject>*>(block_end));
    }
  }

  if (!global_blocks_.IsEmpty()) {
    Address* block_begin = global_blocks_.GetBack();
    Address* block_end = global_next_;
    v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                     reinterpret_cast<Tagged<PyObject>*>(block_end));
  }
}

}  // namespace saauso::internal
