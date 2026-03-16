// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-oddballs.h"

namespace saauso::internal {

BUILTIN(Sysgc) {
  isolate->heap()->CollectGarbage();
  return handle(isolate->py_none_object());
}

}  // namespace saauso::internal
