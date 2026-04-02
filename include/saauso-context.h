// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_CONTEXT_H_
#define INCLUDE_SAAUSO_CONTEXT_H_

#include "saauso-local-handle.h"
#include "saauso-value.h"

namespace saauso {

class Object;
class String;

class Context final : public Value {
 public:
  class Scope {
   public:
    explicit Scope(Local<Context> context);
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    ~Scope();

   private:
    Local<Context> context_;
  };

  // 创建上下文；创建失败返回 Nothing。
  static Local<Context> New(Isolate* isolate);

  // 进入当前上下文。
  void Enter();
  // 退出当前上下文。
  void Exit();
  // 写入变量；写入成功返回 JustVoid，失败返回 Nothing。
  Maybe<void> Set(Local<String> key, Local<Value> value);
  // 读取变量；命中返回 Just(value)，未命中或失败返回 Nothing。
  MaybeLocal<Value> Get(Local<String> key);
  // 获取全局对象；失败返回 Nothing。
  MaybeLocal<Object> Global();

 private:
  friend class Isolate;
  friend class Script;
  friend class ApiBridgeAccess;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_CONTEXT_H_
