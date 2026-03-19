// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_INL_H_
#define SAAUSO_API_API_INL_H_

#include <vector>

#include "include/saauso-embedder.h"
#include "src/execution/isolate.h"

namespace saauso {

struct ApiAccess {
  template <typename T>
  static Local<T> MakeLocal(T* ptr) {
    return Local<T>(ptr);
  }

  static internal::Isolate* UnwrapIsolate(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<internal::Isolate*>(isolate->internal_isolate_);
  }

  static const internal::Isolate* UnwrapIsolate(
      const saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<const internal::Isolate*>(
        isolate->internal_isolate_);
  }

  static std::vector<Value*>* ValueRegistry(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<std::vector<Value*>*>(isolate->values_);
  }

  static const std::vector<Value*>* ValueRegistry(
      const saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<const std::vector<Value*>*>(isolate->values_);
  }

  static void RegisterValue(saauso::Isolate* isolate, Value* value) {
    auto* registry = ValueRegistry(isolate);
    if (registry == nullptr || value == nullptr) {
      return;
    }
    registry->push_back(value);
    value->isolate_ = isolate;
  }

  static void DeleteRegisteredValues(saauso::Isolate* isolate) {
    auto* registry = ValueRegistry(isolate);
    if (registry == nullptr) {
      return;
    }
    registry->clear();
    delete registry;
    isolate->values_ = nullptr;
  }

  static TryCatch* TryCatchTop(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return isolate->try_catch_top_;
  }

  static void SetTryCatchTop(saauso::Isolate* isolate, TryCatch* try_catch) {
    if (isolate == nullptr) {
      return;
    }
    isolate->try_catch_top_ = try_catch;
  }

  static void SetTryCatchException(TryCatch* try_catch,
                                   Local<Value> exception) {
    if (try_catch == nullptr) {
      return;
    }
    try_catch->exception_ = exception;
  }

  static void* GetValueImpl(const Value* value) {
    if (value == nullptr) {
      return nullptr;
    }
    return value->impl_;
  }

  static void SetValueImpl(Value* value, void* impl) {
    if (value == nullptr) {
      return;
    }
    value->impl_ = impl;
  }

  static Isolate* GetValueIsolate(Value* value) {
    if (value == nullptr) {
      return nullptr;
    }
    return value->isolate_;
  }

  static void SetValueIsolate(Value* value, Isolate* isolate) {
    if (value == nullptr) {
      return;
    }
    value->isolate_ = isolate;
  }

  static void SetContextImpl(Context* context, void* impl) {
    if (context == nullptr) {
      return;
    }
    SetValueImpl(static_cast<Value*>(context), impl);
  }

  static void* GetContextImpl(const Context* context) {
    if (context == nullptr) {
      return nullptr;
    }
    return GetValueImpl(static_cast<const Value*>(context));
  }

  static void DeleteValue(Value* value) { delete value; }

  static void SetFunctionCallbackInfoImpl(FunctionCallbackInfo* info,
                                          void* impl) {
    if (info == nullptr) {
      return;
    }
    info->impl_ = impl;
  }

  static void* GetFunctionCallbackInfoImpl(const FunctionCallbackInfo* info) {
    if (info == nullptr) {
      return nullptr;
    }
    return info->impl_;
  }
};

}  // namespace saauso

#endif  // SAAUSO_API_API_INL_H_
