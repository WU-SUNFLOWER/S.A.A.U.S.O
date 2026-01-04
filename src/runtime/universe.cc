// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "runtime/universe.h"

#include "heap/heap.h"
#include "objects/object-shape.h"

namespace saauso::internal {

// static
void Universe::Genesis() {
  heap_ = new Heap();

  array_list_shape_ = heap_->Allocate<ObjectShape>();
  string_shape_ = heap_->Allocate<ObjectShape>();
  hash_table_shape_ = heap_->Allocate<ObjectShape>();
}

// static
void Universe::Destroy() {}

}  // namespace saauso::internal
