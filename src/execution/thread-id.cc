// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/thread-id.h"

#include <atomic>
#include <cassert>

namespace saauso::internal {

namespace {

thread_local int current_thread_id = 0;

std::atomic<int> g_next_thread_id{1};

}  // namespace

// static
ThreadId ThreadId::TryGetCurrent() {
  return current_thread_id == 0 ? Invalid() : ThreadId(current_thread_id);
}

// static
int ThreadId::GetCurrentThreadId() {
  if (current_thread_id == 0) {
    current_thread_id = g_next_thread_id.fetch_add(1);
    assert(1 <= current_thread_id);
  }
  return current_thread_id;
}

}  // namespace saauso::internal
