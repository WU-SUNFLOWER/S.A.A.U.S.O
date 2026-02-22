// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
#define SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_

#include "src/execution/exception-types.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class PyString;

// 统一异常抛出 API
void Runtime_ThrowError(ExceptionType type, const char* message);
void Runtime_ThrowErrorf(ExceptionType type, const char* fmt, ...);

Handle<PyString> Runtime_FormatPendingExceptionForStderr();

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
