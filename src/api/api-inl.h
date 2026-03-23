// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_INL_H_
#define SAAUSO_API_API_INL_H_

#include <vector>

#include "include/saauso-context.h"
#include "include/saauso-exception.h"
#include "include/saauso-function-callback.h"
#include "include/saauso-isolate.h"
#include "include/saauso-local-handle.h"
#include "src/common/globals.h"
#include "src/execution/isolate.h"

namespace saauso {

struct ApiAccess {
  static void SetFunctionCallbackInfoImpl(FunctionCallbackInfo* info,
                                          void* impl) {
    if (info == nullptr) {
      return;
    }
    info->impl_ = impl;
  }

  static void SetTryCatchException(TryCatch* try_catch,
                                   Local<Value> exception) {
    if (try_catch == nullptr) {
      return;
    }
    try_catch->exception_ = exception;
  }
};

}  // namespace saauso

#endif  // SAAUSO_API_API_INL_H_
