// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_ISOLATE_H_
#define INCLUDE_SAAUSO_ISOLATE_H_

#include "saauso-exception.h"
#include "saauso-value.h"

namespace saauso {

struct IsolateCreateParams {};

class Isolate {
 public:
  // Isolate::Scope 是一个 RAII 辅助类，用于在当前线程中进入/退出指定的
  // Isolate。 在构造时调用 Enter()，析构时调用 Exit()。
  class Scope {
    explicit Scope(Isolate* isolate) : isolate_(isolate) { isolate_->Enter(); }

    ~Scope() { isolate_->Exit(); }

    // 我们不允许对 Isolate::Scope 进行拷贝
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

   private:
    Isolate* const isolate_;
  };

  static Isolate* New(
      const IsolateCreateParams& params = IsolateCreateParams());

  Isolate() = delete;
  ~Isolate() = delete;
  Isolate(const Isolate&) = delete;
  Isolate& operator=(const Isolate&) = delete;

  void Enter();
  void Exit();

  void Dispose();
  void ThrowException(Local<Value> exception);
  size_t ValueRegistrySizeForTesting() const;

  void* operator new(size_t size) = delete;
  void* operator new[](size_t size) = delete;
  void operator delete(void*, size_t) = delete;
  void operator delete[](void*, size_t) = delete;
};

}  // namespace saauso

#endif
