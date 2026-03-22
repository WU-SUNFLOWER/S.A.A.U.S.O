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
  static Local<Object> New(Isolate* isolate);
  bool Set(Local<String> key, Local<Value> value);
  MaybeLocal<Value> Get(Local<String> key);
  MaybeLocal<Value> CallMethod(Local<Context> context,
                               Local<String> name,
                               int argc,
                               Local<Value> argv[]);
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_OBJECT_H_
