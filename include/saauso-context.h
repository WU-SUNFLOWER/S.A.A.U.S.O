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
  static Local<Context> New(Isolate* isolate);

  void Enter();
  void Exit();
  bool Set(Local<String> key, Local<Value> value);
  MaybeLocal<Value> Get(Local<String> key);
  Local<Object> Global();

 private:
  friend class Isolate;
  friend class Script;
  friend struct ApiAccess;
};

class ContextScope {
 public:
  explicit ContextScope(Local<Context> context);
  ContextScope(const ContextScope&) = delete;
  ContextScope& operator=(const ContextScope&) = delete;
  ~ContextScope();

 private:
  Local<Context> context_;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_CONTEXT_H_
