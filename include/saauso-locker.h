// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_LOCKER_H_
#define INCLUDE_SAAUSO_LOCKER_H_

namespace saauso {

class Isolate;

// Locker 用于多线程环境下的同步。
// 它确保在同一时刻只有一个线程可以访问该 Isolate（类似于 GIL 的作用范围，但在
// Isolate 级别）。
//
// 在多个线程竞争使用同一个 Isolate 的场景中，我们必须按类似以下的形式编写代码：
// saauso::Isolate* = saauso::Isolate::New();
// {
//   saauso::Locker lock(isolate); // 获取锁，如果被占用则阻塞
//   saauso::Scope scope(isolate); // 进入作用域
//   // 安全地使用 isolate...
// }
//
// 需指出的是，虽然 S.A.A.U.S.O 中提供了互斥锁 Locker，在实际使用场景中，我们
// 仍然强烈推荐为每个 Worker Thread 分配独立的 Isolate 实例，而非让多个线程抢
// 占单个 Isolate。
class Locker {
 public:
  // Locker 被构造出来后，会自动给指定的 Isolate 加锁。
  // 注意，在 S.A.A.U.S.O 中我们允许单个线程对同一个 Isolate 重复加锁，
  // 即 saauso::Locker 是一把重入锁。
  explicit Locker(Isolate* isolate);

  // Locker 被析构后，会自动给当前 Isolate 解锁。
  ~Locker();

  // 检测指定 Isolate 是否被指定线程锁住
  static bool IsLocked(Isolate* isolate);

  // Disallow copying and assigning.
  Locker(const Locker&) = delete;
  void operator=(const Locker&) = delete;

 private:
  Isolate* isolate_;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_LOCKER_H_
