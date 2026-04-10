// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_API_API_EXCEPTION_SUPPORT_H_
#define SAAUSO_API_API_EXCEPTION_SUPPORT_H_

#include "src/execution/isolate.h"

namespace saauso {
namespace api {

bool TryToCapturePendingExceptionToEembedderTryCatch(i::Isolate* isolate);
bool FinalizePendingExceptionAtApiBoundary(i::Isolate* isolate);

}  // namespace api
}  // namespace saauso

#endif
