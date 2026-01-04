// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "heap/heap.h"

#include <cstdlib>

namespace saauso::internal {

void* Heap::AllocateRaw(size_t size_in_bytes) {
  return malloc(size_in_bytes);
}

}  // namespace saauso::internal
