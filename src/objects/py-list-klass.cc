// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-list-klass.h"

#include "heap/heap.h"
#include "objects/py-string.h"
#include "runtime/universe.h"

namespace saauso::internal {

PyListKlass* PyListKlass::instance_ = nullptr;

// static
PyListKlass* PyListKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::Initialize() {
  // TODO: 初始化虚函数表

  // 设置类名
  set_name(PyString::NewInstance("list"));
}

}  // namespace saauso::internal
