// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_CONTAINER_H_
#define INCLUDE_SAAUSO_CONTAINER_H_

#include "saauso-local-handle.h"
#include "saauso-value.h"

namespace saauso {

class List : public Value {
 public:
  // 创建列表；创建失败返回 Nothing。
  static MaybeLocal<List> New(Isolate* isolate);
  int64_t Length() const;
  // 追加元素；成功返回 JustVoid，失败返回 Nothing。
  Maybe<void> Push(Local<Value> value);
  // 按下标写入；成功返回 JustVoid，失败返回 Nothing。
  Maybe<void> Set(int64_t index, Local<Value> value);
  // 按下标读取；命中返回 Just(value)，越界或失败返回 Nothing。
  MaybeLocal<Value> Get(int64_t index) const;
};

class Tuple : public Value {
 public:
  // 创建元组；创建失败返回 Nothing。
  static MaybeLocal<Tuple> New(Isolate* isolate, int argc, Local<Value> argv[]);
  int64_t Length() const;
  // 按下标读取；命中返回 Just(value)，越界或失败返回 Nothing。
  MaybeLocal<Value> Get(int64_t index) const;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_CONTAINER_H_
