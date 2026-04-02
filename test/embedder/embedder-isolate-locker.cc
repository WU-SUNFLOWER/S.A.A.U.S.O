// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <chrono>
#include <thread>

#include "saauso.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso {

TEST(IsolateLocker, LockerBlocksOtherThreadUntilRelease) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  std::atomic<bool> thread2_started{false};
  std::atomic<bool> release_thread1{false};
  std::atomic<int> phase{0};

  std::thread thread1([&] {
    Locker locker(isolate);
    phase.store(1, std::memory_order_release);
    while (!release_thread1.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  // 自旋等待 thread1 启动
  while (phase.load(std::memory_order_acquire) != 1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  std::thread thread2([&] {
    thread2_started.store(true, std::memory_order_release);
    Locker locker(isolate);
    phase.store(2, std::memory_order_release);
  });

  // 自选等待 thread2 启动
  while (!thread2_started.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // 等待足够久，再检查。
  // 因为 Isolate 实例一开始已经被 thread1 锁住了，thread 2 会被阻塞。
  // 因此在 thread 1 解锁 Isolate 实例之前，thread 2 不可能更新 phase，
  // 即预期 phase 依旧是 1。
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  EXPECT_EQ(phase.load(std::memory_order_acquire), 1);

  // 让 thread 1 退出，然后等待两个线程全部执行完毕并退出
  release_thread1.store(true, std::memory_order_release);
  thread1.join();
  thread2.join();

  // thread 1退出后解锁Isolate实例，之后thread 2能够抢占到该实例并继续执行。
  // 因此这里预期 phase 会被 thread 2 更新为 2。
  EXPECT_EQ(phase.load(std::memory_order_acquire), 2);
  isolate->Dispose();
  Saauso::Dispose();
}

TEST(IsolateLocker, IsLockedReflectsCurrentThreadState) {
  Saauso::Initialize();
  Isolate* isolate = Isolate::New();
  ASSERT_NE(isolate, nullptr);

  EXPECT_FALSE(Locker::IsLocked(isolate));
  {
    Locker locker(isolate);
    EXPECT_TRUE(Locker::IsLocked(isolate));
    {
      // 同一个线程允许对特定的 isolate 实例重复加锁，即允许重入。
      Locker reentrant(isolate);
      EXPECT_TRUE(Locker::IsLocked(isolate));
    }
    EXPECT_TRUE(Locker::IsLocked(isolate));
  }
  EXPECT_FALSE(Locker::IsLocked(isolate));

  isolate->Dispose();
  Saauso::Dispose();
}

TEST(IsolateLockerDeathTest, DestroyLockerFromAnotherThreadShouldDie) {
  ASSERT_DEATH_IF_SUPPORTED(
      {
        Saauso::Initialize();
        Isolate* isolate = Isolate::New();
        Locker* locker = new Locker(isolate);
        // 加锁解锁必须发生在同一个线程，否则直接崩溃！
        std::thread bad_thread([&] { delete locker; });
        bad_thread.join();
      },
      "");
}

TEST(IsolateLockerDeathTest, UnlockWhileStillEnteredShouldDie) {
  ASSERT_DEATH_IF_SUPPORTED(
      {
        Saauso::Initialize();
        Isolate* isolate = Isolate::New();
        Locker* locker = new Locker(isolate);
        Isolate::Scope isolate_scope(isolate);
        // 解锁之前必须先退出 isolate，否则直接崩溃！
        delete locker;
      },
      "");
}

}  // namespace saauso
