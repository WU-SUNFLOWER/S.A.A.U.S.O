// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
#define SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyObject;
class PyString;

void Runtime_ThrowTypeError(const char* message);
void Runtime_ThrowRuntimeError(const char* message);
void Runtime_ThrowValueError(const char* message);
void Runtime_ThrowIndexError(const char* message);
void Runtime_ThrowKeyError(const char* message);

void Runtime_ThrowTypeErrorf(const char* fmt, ...);
void Runtime_ThrowValueErrorf(const char* fmt, ...);
void Runtime_ThrowIndexErrorf(const char* fmt, ...);
void Runtime_ThrowKeyErrorf(const char* fmt, ...);

Handle<PyString> Runtime_FormatPendingExceptionForStderr();

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
