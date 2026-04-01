// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "include/saauso-function-callback.h"
#include "include/saauso-maybe.h"
#include "include/saauso-value.h"
#include "src/api/api-support.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso {

int FunctionCallbackInfo::Length() const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr || impl->args.is_null()) {
    return 0;
  }
  return static_cast<int>(impl->args->length());
}

MaybeLocal<Value> FunctionCallbackInfo::operator[](int index) const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr || impl->args.is_null() || index < 0 ||
      index >= impl->args->length()) {
    return MaybeLocal<Value>();
  }
  return api::Utils::ToLocal<Value>(impl->args->Get(index, impl->isolate));
}

Maybe<int64_t> FunctionCallbackInfo::GetIntegerArg(int index) const {
  MaybeLocal<Value> maybe_value = operator[](index);
  Local<Value> value;
  if (!maybe_value.ToLocal(&value)) {
    return i::kNullMaybe;
  }
  return value->ToInteger();
}

Maybe<std::string> FunctionCallbackInfo::GetStringArg(int index) const {
  MaybeLocal<Value> maybe_value = operator[](index);
  Local<Value> value;
  if (!maybe_value.ToLocal(&value)) {
    return i::kNullMaybe;
  }
  return value->ToString();
}

MaybeLocal<Value> FunctionCallbackInfo::Receiver() const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr) {
    return MaybeLocal<Value>();
  }
  return api::Utils::ToLocal<Value>(impl->receiver);
}

Isolate* FunctionCallbackInfo::GetIsolate() const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<Isolate*>(impl->isolate);
}

void FunctionCallbackInfo::ThrowRuntimeError(std::string_view message) const {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr || impl->isolate == nullptr) {
    return;
  }

  auto* i_isolate = reinterpret_cast<i::Isolate*>(impl->isolate);
  if (i_isolate == nullptr) {
    return;
  }

  i::HandleScope handle_scope(i_isolate);
  std::string error(message);
  i::Runtime_ThrowError(i_isolate, i::ExceptionType::kRuntimeError,
                        error.c_str());
}

void FunctionCallbackInfo::SetReturnValue(Local<Value> value) {
  auto* impl = reinterpret_cast<api::FunctionCallbackInfoImpl*>(impl_);
  if (impl == nullptr) {
    return;
  }
  impl->return_value = value;
}

}  // namespace saauso
