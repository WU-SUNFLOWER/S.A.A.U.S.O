// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
#define SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_

#include "src/execution/exception-types.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
class PyString;

// 统一异常抛出 API
void Runtime_ThrowError(ExceptionType type, const char* message = nullptr);
void Runtime_ThrowErrorf(ExceptionType type, const char* fmt, ...);

// 将当前解释器中pending的异常导出为形如`异常类型名: 异常内容`字符串。
// 如果当前解释器中不存在pending的异常，将返回一个空字符串。
//
// 该方法主要用于：
// - 打印未被捕获的一场到控制台
// - 验证单元测试的抛出异常内容是否正确
Handle<PyString> Runtime_FormatPendingExceptionForStderr();

// 若当前 pending 异常为 StopIteration，则清除该异常并返回 true（用于 for 循环
// 正常结束迭代）；否则不修改状态并返回 false。调用方需在 next_result 为 null 时
// 根据返回值决定是正常退出循环还是进入异常展开。
bool Runtime_ConsumePendingStopIterationIfSet(Isolate* isolate);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_EXCEPTIONS_H_
