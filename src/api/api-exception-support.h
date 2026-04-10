// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_EXCEPTION_SUPPORT_H_
#define SAAUSO_API_API_EXCEPTION_SUPPORT_H_

#include "include/saauso-local-handle.h"
#include "src/execution/isolate.h"

namespace saauso {
namespace api {

bool TryToCapturePendingExceptionToEembedderTryCatch(i::Isolate* isolate);
bool FinalizePendingExceptionAtApiBoundary(i::Isolate* isolate);
Maybe<void> FinalizePendingExceptionAtApiBoundaryAndReturnNothing(
    i::Isolate* isolate);

template <typename T>
MaybeLocal<T> FinalizePendingExceptionAtApiBoundaryAndReturnEmptyLocal(
    i::Isolate* isolate) {
  FinalizePendingExceptionAtApiBoundary(isolate);
  return MaybeLocal<T>();
}

}  // namespace api
}  // namespace saauso

#endif
