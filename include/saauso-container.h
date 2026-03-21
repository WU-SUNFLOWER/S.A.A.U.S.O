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
  static Local<List> New(Isolate* isolate);
  int64_t Length() const;
  bool Push(Local<Value> value);
  bool Set(int64_t index, Local<Value> value);
  MaybeLocal<Value> Get(int64_t index) const;
};

class Tuple : public Value {
 public:
  static MaybeLocal<Tuple> New(Isolate* isolate, int argc, Local<Value> argv[]);
  int64_t Length() const;
  MaybeLocal<Value> Get(int64_t index) const;
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_CONTAINER_H_
