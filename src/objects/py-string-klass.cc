// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-string-klass.h"

#include "heap/heap.h"
#include "objects/py-string.h"
#include "runtime/universe.h"

namespace saauso::internal {

PyStringKlass* PyStringKlass::instance_ = nullptr;

// static
PyStringKlass* PyStringKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyStringKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyStringKlass::Initialize() {
  // TODO: 初始化虚函数表

  // 设置类名
  set_name(PyString::NewInstance("str"));
}

}  // namespace saauso::internal
