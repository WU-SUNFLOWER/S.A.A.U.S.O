// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/thread-state-infras.h"

#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"

namespace saauso {

namespace internal {

ThreadManager::ThreadManager(Isolate* isolate) : isolate_(isolate) {
  assert(isolate_ != nullptr);
}

void ThreadManager::Lock() {
  // 因为下面要对 Isolate 内部的字段进行操作，这里首先加锁
  mutex_.lock();

  ThreadId tid = ThreadId::Current();
  // 仅允许同一个线程重入对 Isolate 加锁，否则直接崩溃
  if (mutex_owner_.load() != ThreadId{} && mutex_owner_.load() != tid) {
    // 其他情况，直接让进程崩溃
    std::printf(
        "Illegally locking isolate, since this isolate is alread locked by "
        "other thread.");
    std::exit(1);
  }

  // 标记该 Isolate 被当前线程占用
  mutex_owner_.store(tid);

  // 该 Isolate 的加锁次数（含重入）自增 1
  ++mutex_count_;
}

void ThreadManager::Unlock() {
  // 加锁和解锁该 Isolate 的必须是同一个线程
  if (mutex_owner_.load() != ThreadId::Current()) {
    std::printf(
        "Illegally unlocking isolate, since you should unlock the isolate in "
        "the thread which locked it.");
  }

  // 该 Isolate 的加锁次数（含重入）不能已经清零
  assert(mutex_count_ > 0);

  --mutex_count_;

  // 当加锁次数清空时，说明当前线程已经彻底释放对该 Isolate 的占用。
  // 此时清除 locker_thread_ 标记。
  if (mutex_count_ == 0) {
    // 在释放锁之前，当前线程必须已经完全退出该 Isolate
    assert(isolate_->entry_count() == 0);
    mutex_owner_.store(ThreadId{});
  }

  mutex_.unlock();
}

}  // namespace internal

}  // namespace saauso
