// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_
#define SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_

#include "src/handles/handles.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class HandleScopeImplementer {
 public:
  // 一个block可以装载的Address数量
  static constexpr size_t kHandleBlockSize = 512;

  HandleScopeImplementer() = default;
  ~HandleScopeImplementer();

  HandleScope::State* CurrentHandleScopeState() const;

  int NumberOfHandles();

  Address* AllocateSpareOrNewBlock();
  void ReleaseSpareAndExtendedBlocks(int n);

  Vector<Address*>& blocks() { return blocks_; }

 private:
  Address* spare_{nullptr};
  Vector<Address*> blocks_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_HANDLES_HANDLE_SCOPE_IMPLEMENTER_H_
