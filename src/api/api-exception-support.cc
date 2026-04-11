// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/api/api-exception-support.h"

#include "src/api/api-bridge-access.h"
#include "src/api/api-handle-utils.h"

namespace saauso {
namespace api {

bool TryToForwardPendingExceptionToEembedderTryCatch(i::Isolate* isolate) {
  assert(isolate->HasPendingException());

  TryCatch* try_catch = isolate->try_catch_top();
  if (try_catch == nullptr) {
    return false;
  }

  // 防止位于调用链较浅处的 saauso::TryCatch，
  // 越过调用链更深处的的 Python try...catch代码块，提前捕获异常。
  if (ApiBridgeAccess::GetTryCatchPythonExecutionDepth(try_catch) <
      isolate->python_execution_depth()) {
    return false;
  }

  i::Handle<i::PyObject> exception =
      isolate->exception_state()->pending_exception(isolate);
  ApiBridgeAccess::SetTryCatchException(try_catch, exception);
  isolate->exception_state()->Clear();
  return true;
}

bool FinalizePendingExceptionAtApiBoundary(i::Isolate* isolate) {
  if (!isolate->HasPendingException()) {
    return false;
  }

  // 先尝试查找是否有合适的 saauso::TryCatch，可以把异常转发过去
  if (TryToForwardPendingExceptionToEembedderTryCatch(isolate)) {
    return true;
  }

  // 当不存在合适的 saauso::TryCatch 时：
  // - 如果 Python 调用的深度为0（即当前调用链没有进入任何Python调用）：
  //   - 直接清除掉 Isolate 上的异常。
  //   - 异常清除掉后，宿主C++程序仍然可以通过Embedder API的返回值为
  //     空 Maybe/MaybeHandle 来获知有异常发生
  // - 如果 Python 调用的深度>0：
  //   - 尽管当前TryCatch不存在，但我们无法确定更外层的调用链中，
  //     Python代码中是否存在try...catch块。
  //   - 因此当前不应该清除Isolate上的异常状态。
  if (isolate->python_execution_depth() == 0) {
    isolate->exception_state()->Clear();
  }

  return true;
}

}  // namespace api
}  // namespace saauso
