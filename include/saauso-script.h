// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_SCRIPT_H_
#define INCLUDE_SAAUSO_SCRIPT_H_

#include "saauso-local-handle.h"
#include "saauso-value.h"

namespace saauso {

class String;
class Context;

class Script final : public Value {
 public:
  static MaybeLocal<Script> Compile(Isolate* isolate, Local<String> source);

  MaybeLocal<Value> Run(Local<Context> context);
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_SCRIPT_H_
