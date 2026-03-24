// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <mutex>

#include "src/execution/thread-id.h"

namespace saauso {
namespace internal {

class Isolate;

class ThreadManager {
 public:
  explicit ThreadManager(Isolate* isolate);

  void Lock();
  void Unlock();

 private:
  Isolate* isolate_{nullptr};

  std::recursive_mutex mutex_;
  std::atomic<ThreadId> mutex_owner_;
  int mutex_count_{0};
};

}  // namespace internal

}  // namespace saauso
