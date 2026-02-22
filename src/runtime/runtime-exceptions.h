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

// 定义所有支持的异常类型枚举。
// 新增异常类型只需在此处添加枚举值，并在 runtime-exceptions.cc 的
// GetExceptionStringHandle 中添加对应映射即可。
enum class ExceptionType {
  kTypeError,
  kRuntimeError,
  kValueError,
  kIndexError,
  kKeyError,
  kNameError,
  // 预留给未来扩展
  kAttributeError,
  kZeroDivisionError,
  kStopIteration,
};

// 统一异常抛出 API
void Runtime_ThrowError(ExceptionType type, const char* message);
void Runtime_ThrowErrorf(ExceptionType type, const char* fmt, ...);

Handle<PyString> Runtime_FormatPendingExceptionForStderr();

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
