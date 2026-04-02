// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-locker.h"
#include "src/execution/isolate.h"
#include "src/execution/thread-state-infras.h"

namespace saauso {

Locker::Locker(Isolate* isolate) : isolate_(isolate) {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  i_isolate->thread_manager()->Lock();
}

Locker::~Locker() {
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate_);
  i_isolate->thread_manager()->Unlock();
}

bool Locker::IsLocked(Isolate* isolate) {
  assert(isolate != nullptr);
  auto* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  return i_isolate->thread_manager()->IsLockedByCurrentThread();
}

}  // namespace saauso
