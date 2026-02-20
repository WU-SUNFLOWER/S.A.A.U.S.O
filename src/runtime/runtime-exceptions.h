// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_RUNTIME_RUNTIME_EXCEPTIONS_H_
#define SRC_RUNTIME_RUNTIME_EXCEPTIONS_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyString;

Handle<PyObject> Runtime_NewExceptionInstance(Handle<PyString> exception_type_name,
                                              Handle<PyString> message_or_null);

void Runtime_ThrowNewException(Handle<PyString> exception_type_name,
                               Handle<PyString> message_or_null);

void Runtime_ThrowTypeError(const char* message);
void Runtime_ThrowRuntimeError(const char* message);

void Runtime_ThrowTypeErrorf(const char* fmt, ...);

Handle<PyString> Runtime_FormatPendingExceptionForStderr();

}  // namespace saauso::internal

#endif  // SRC_RUNTIME_RUNTIME_EXCEPTIONS_H_

