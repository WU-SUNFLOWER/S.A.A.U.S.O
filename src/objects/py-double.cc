// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-double.h"

#include "handles/handles.h"
#include "heap/heap.h"
#include "runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyDouble> PyDouble::NewInstance(double value) {
  Handle<PyDouble> object(Universe::heap_->Allocate<PyDouble>());
  object->value_ = value;

  return object;
}

}  // namespace saauso::internal
