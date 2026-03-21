// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_
#define INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "saauso-local-handle.h"

namespace saauso {

class Value;
class Isolate;

class FunctionCallbackInfo {
 public:
  FunctionCallbackInfo() = default;

  int Length() const;
  Local<Value> operator[](int index) const;
  bool GetIntegerArg(int index, int64_t* out) const;
  bool GetStringArg(int index, std::string* out) const;
  Local<Value> Receiver() const;
  Isolate* GetIsolate() const;
  void ThrowRuntimeError(std::string_view message) const;
  void SetReturnValue(Local<Value> value);

 private:
  void* impl_{nullptr};

  friend struct ApiAccess;
};

using FunctionCallback = void (*)(FunctionCallbackInfo& info);

}  // namespace saauso

#endif  // INCLUDE_SAAUSO_FUNCTION_CALLBACKS_H_
