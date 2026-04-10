// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_BRIDGE_ACCESS_H_
#define SAAUSO_API_API_BRIDGE_ACCESS_H_

#include <cassert>

#include "include/saauso-exception.h"
#include "include/saauso-function-callback.h"
#include "include/saauso-local-handle.h"
#include "include/saauso-value.h"
#include "src/handles/handles.h"
#include "src/objects/py-object.h"

namespace saauso {
namespace api {

class ApiBridgeAccess final {
 public:
  static void SetFunctionCallbackInfoImpl(FunctionCallbackInfo* info,
                                          void* impl) {
    assert(info != nullptr);
    info->impl_ = impl;
  }

  static void SetTryCatchException(TryCatch* try_catch,
                                   i::Handle<internal::PyObject> exception) {
    assert(try_catch != nullptr);
    try_catch->SetException(exception);
  }

  static int GetTryCatchPythonExecutionDepth(const TryCatch* try_catch) {
    assert(try_catch != nullptr);
    return try_catch->PythonExecutionDepth();
  }
};

}  // namespace api
}  // namespace saauso

#endif  // SAAUSO_API_API_BRIDGE_ACCESS_H_
