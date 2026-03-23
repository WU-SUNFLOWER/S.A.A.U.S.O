// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_OBJECT_H_
#define INCLUDE_SAAUSO_OBJECT_H_

#include "saauso-local-handle.h"
#include "saauso-value.h"

namespace saauso {

class String;
class Context;

class Object : public Value {
 public:
  // 创建对象；创建失败返回 Nothing。
  static MaybeLocal<Object> New(Isolate* isolate);
  // 写入属性；成功返回 JustVoid，失败返回 Nothing。
  Maybe<void> Set(Local<String> key, Local<Value> value);
  // 读取属性；命中返回 Just(value)，未命中或失败返回 Nothing。
  MaybeLocal<Value> Get(Local<String> key);
  // 调用对象方法；调用成功返回 Just(value)，失败返回 Nothing。
  MaybeLocal<Value> CallMethod(Local<Context> context,
                               Local<String> name,
                               int argc,
                               Local<Value> argv[]);
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_OBJECT_H_
