// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-oddballs-klass.h"

#include "heap/heap.h"
#include "objects/py-float.h"
#include "objects/py-string.h"
#include "runtime/universe.h"
#include "utils/utils.h"

namespace saauso::internal {

PyBooleanKlass* PyBooleanKlass::instance_ = nullptr;
PyNoneKlass* PyNoneKlass::instance_ = nullptr;

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
PyBooleanKlass* PyBooleanKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyBooleanKlass::Initialize() {
  // TODO: 初始化虚函数表

  // 设置类名
  set_name(PyString::NewInstance("bool"));
}


///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
PyNoneKlass* PyNoneKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyNoneKlass::Initialize() {
  // TODO: 初始化虚函数表

  // 设置类名
  set_name(PyString::NewInstance("NoneType"));
}

}  // namespace saauso::internal
