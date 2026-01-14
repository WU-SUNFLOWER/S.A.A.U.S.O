// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object-klass.h"

#include "src/heap/heap.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Tagged<PyObjectKlass> PyObjectKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyObjectKlass::Initialize() {}

// static
void PyObjectKlass::Finalize() {
  instance_ = Tagged<PyObjectKlass>::Null();
}

}  // namespace saauso::internal
