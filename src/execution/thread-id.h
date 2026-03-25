// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_EXECUTION_THREAD_ID_H_
#define SAAUSO_EXECUTION_THREAD_ID_H_

namespace saauso::internal {

// 平台无关的线程标识符（线程 ID）
class ThreadId {
 public:
  // 默认创建一个无效的、不代表任何具体线程的 ID
  constexpr ThreadId() noexcept : ThreadId(kInvalidId) {}

  bool operator==(const ThreadId& other) const { return id_ == other.id_; }
  bool operator!=(const ThreadId& other) const { return id_ != other.id_; }

  // 检查当前线程 ID 是否有效
  bool IsValid() const { return id_ != kInvalidId; }

  // 将当前线程 ID 转为 int 表示形式
  constexpr int ToInteger() const { return id_; }

  // 如果调用方线程已经被分配了有效的 ID，则返回。
  // 否则返回一个无效的 ThreadId。
  static ThreadId TryGetCurrent();

  // 获取当前线程的 ID。
  // 如果当前线程还没有被分配 ID，则先执行分配，再返回。
  static ThreadId Current() { return ThreadId(GetCurrentThreadId()); }

  // 创建一个无效的 ThreadId，语义等同于ThreadId()
  static constexpr ThreadId Invalid() { return ThreadId(kInvalidId); }

  // 将一个 int 包装为 ThreadId
  static constexpr ThreadId FromInteger(int id) { return ThreadId(id); }

 private:
  static constexpr int kInvalidId = -1;

  explicit constexpr ThreadId(int id) noexcept : id_(id) {}

  static int GetCurrentThreadId();

  int id_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_EXECUTION_THREAD_ID_H_
