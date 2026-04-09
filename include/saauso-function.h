// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_FUNCTION_H_
#define INCLUDE_SAAUSO_FUNCTION_H_

#include <string_view>

#include "saauso-function-callback.h"
#include "saauso-value.h"

namespace saauso {

class Context;

class Function final : public Value {
 public:
  // 创建宿主回调函数；创建失败返回 Nothing。
  static Local<Function> New(Isolate* isolate,
                             FunctionCallback callback,
                             std::string_view name);

  // 调用函数；调用成功返回 Just(value)，失败返回 Nothing。
  MaybeLocal<Value> Call(Local<Context> context,
                         Local<Value> receiver,
                         int argc,
                         Local<Value> argv[]);
};

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_FUNCTION_H_
