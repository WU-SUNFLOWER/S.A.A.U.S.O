// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_INL_H_
#define SAAUSO_API_API_INL_H_

#include <vector>

#include "include/saauso-embedder.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"

namespace saauso {

struct ApiAccess {
  template <typename T>
  static Local<T> MakeLocal(T* ptr) {
    return Local<T>(ptr);
  }

  static i::Isolate* UnwrapIsolate(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<i::Isolate*>(isolate->internal_isolate_);
  }

  static const i::Isolate* UnwrapIsolate(const saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<const i::Isolate*>(isolate->internal_isolate_);
  }

  static std::vector<Context*>* EnteredContextStack(saauso::Isolate* isolate) {
    if (isolate == nullptr) {
      return nullptr;
    }
    return reinterpret_cast<std::vector<Context*>*>(isolate->entered_contexts_);
  }

  static void DeleteEnteredContextStack(saauso::Isolate* isolate) {
    auto* entered_contexts = EnteredContextStack(isolate);
    if (entered_contexts == nullptr) {
      return;
    }
    entered_contexts->clear();
    delete entered_contexts;
    isolate->entered_contexts_ = nullptr;
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
