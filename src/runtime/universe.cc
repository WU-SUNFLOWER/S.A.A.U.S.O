// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "runtime/universe.h"

#include "heap/heap.h"
#include "objects/py-boolean.h"

namespace saauso::internal {

// static
void Universe::Genesis() {
  heap_ = new Heap();

  py_true_object_ = PyBoolean::NewInstance(true);
  py_false_object_ = PyBoolean::NewInstance(false);
}

// static
void Universe::Destroy() {}

}  // namespace saauso::internal
