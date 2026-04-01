// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_
#define INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "saauso-local-handle.h"
#include "saauso-maybe.h"

namespace saauso {

class Value;
class Isolate;

namespace api {
class ApiBridgeAccess;
}

class FunctionCallbackInfo {
 public:
  FunctionCallbackInfo() = default;

  int Length() const;
  // 获取第 index 个参数；越界或不可用时返回 Nothing。
  MaybeLocal<Value> operator[](int index) const;
  // 获取第 index 个参数并转换为整数；越界或类型不匹配时返回 Nothing。
  Maybe<int64_t> GetIntegerArg(int index) const;
  // 获取第 index 个参数并转换为字符串；越界或类型不匹配时返回 Nothing。
  Maybe<std::string> GetStringArg(int index) const;
  // 获取接收者对象；不可用时返回 Nothing。
  MaybeLocal<Value> Receiver() const;
  Isolate* GetIsolate() const;
  void ThrowRuntimeError(std::string_view message) const;
  // 设置回调返回值。未设置时回调返回 None。
  void SetReturnValue(Local<Value> value);

 private:
  friend class api::ApiBridgeAccess;

  void* impl_{nullptr};
};

using FunctionCallback = void (*)(FunctionCallbackInfo& info);

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_
