// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/api/api-exception-support.h"

#include "src/api/api-bridge-access.h"
#include "src/api/api-handle-utils.h"

namespace saauso {
namespace api {

bool CapturePendingException(i::Isolate* isolate) {
  if (!isolate->HasPendingException()) {
    return false;
  }

  TryCatch* try_catch = isolate->try_catch_top();
  if (try_catch == nullptr) {
    return true;
  }

  i::Handle<i::PyObject> exception =
      isolate->exception_state()->pending_exception(isolate);
  ApiBridgeAccess::SetTryCatchException(try_catch,
                                        api::Utils::ToLocal<Value>(exception));
  isolate->exception_state()->Clear();
  return true;
}

}  // namespace api
}  // namespace saauso
