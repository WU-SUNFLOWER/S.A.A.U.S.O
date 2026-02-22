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

// 将当前解释器中pending的异常导出为形如`异常类型名: 异常内容`字符串。
// 如果当前解释器中不存在pending的异常，将返回一个空字符串。
//
// 该方法主要用于：
// - 打印未被捕获的一场到控制台
// - 验证单元测试的抛出异常内容是否正确
Handle<PyString> Runtime_FormatPendingExceptionForStderr();

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
