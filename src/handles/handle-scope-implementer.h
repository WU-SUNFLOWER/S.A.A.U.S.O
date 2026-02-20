// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_
#define SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_

#include "src/handles/handles.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class ObjectVisitor;

class HandleScopeImplementer {
 public:
  // 一个block可以装载的Address数量
  static constexpr size_t kHandleBlockSize = 512;

  explicit HandleScopeImplementer(Isolate* isolate);
  ~HandleScopeImplementer();

  HandleScope::State* handle_scope_state();
  void set_handle_scope_state(HandleScope::State* state);

  int NumberOfHandles();
  int NumberOfGlobalHandles() const { return global_handle_count_; }

  Address* AllocateSpareOrNewBlock();
  void ReleaseSpareAndExtendedBlocks(int n);

  Address* CreateGlobalHandle(Address ptr);
  void DestroyGlobalHandle(Address* location);

  void Iterate(ObjectVisitor* v);

  Vector<Address*>& blocks() { return blocks_; }

  Isolate* isolate() const { return isolate_; }

 private:
  Isolate* isolate_;

  HandleScope::State handle_scope_state_;

  Address* spare_{nullptr};
  Vector<Address*> blocks_;

  int global_handle_count_{0};
  Vector<Address*> global_blocks_;
  Vector<Address*> global_free_slot_list_;
  Address* global_next_{nullptr};
  Address* global_limit_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_
