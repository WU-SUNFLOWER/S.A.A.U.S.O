// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-exceptions.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

// 创建一个新的异常实例。
// - 失败时返回 empty，并保证已设置 pending exception。
MaybeHandle<PyObject> NewExceptionInstance(Handle<PyString> exception_type_name,
                                           Handle<PyString> message_or_null) {
  Handle<PyDict> builtins = Execution::builtins(Isolate::Current());
  Handle<PyObject> exception_type = builtins->Get(exception_type_name);
  assert(!exception_type.is_null());

  Handle<PyObject> exception = Handle<PyObject>::null();
  ASSIGN_RETURN_ON_EXCEPTION(
      Isolate::Current(), exception,
      Runtime_NewObject(Handle<PyTypeObject>::cast(exception_type),
                        Handle<PyObject>::null(), Handle<PyObject>::null()));

  if (!message_or_null.is_null()) {
    Handle<PyDict> properties = PyObject::GetProperties(exception);
    if (!properties.is_null()) {
      PyDict::Put(properties, ST(message), message_or_null);
    }
  }

  return exception;
}

void ThrowNewException(Handle<PyString> exception_type_name,
                       Handle<PyString> message_or_null) {
  auto* state = Isolate::Current()->exception_state();
  if (state->HasPendingException()) {
    return;
  }

  auto* isolate = Isolate::Current();
  Handle<PyObject> exception;

  ASSIGN_RETURN_ON_EXCEPTION_VOID(
      isolate, exception,
      NewExceptionInstance(exception_type_name, message_or_null));

  state->Throw(*exception);
}

// 辅助函数：根据枚举类型获取对应的异常类名字符串 Handle
Handle<PyString> GetExceptionStringHandle(ExceptionType type) {
  switch (type) {
    case ExceptionType::kTypeError:
      return ST(type_err);
    case ExceptionType::kRuntimeError:
      return ST(runtime_err);
    case ExceptionType::kValueError:
      return ST(value_err);
    case ExceptionType::kIndexError:
      return ST(index_err);
    case ExceptionType::kKeyError:
      return ST(key_err);
    case ExceptionType::kNameError:
      return ST(name_err);
    case ExceptionType::kAttributeError:
      return ST(attr_err);
    case ExceptionType::kZeroDivisionError:
      return ST(div_zero);
    case ExceptionType::kStopIteration:
      return ST(stop_iter);
    default:
      // 默认回退到 RuntimeError
      return ST(runtime_err);
  }
}

// 将 va_list 格式化为字符串并抛出指定类型的异常。
// 内部复用栈上 256 字节缓冲，仅在超长消息时回退到堆分配。
void ThrowFormattedError(ExceptionType type, const char* fmt, va_list ap) {
  char buf[256];
  va_list ap_copy;
  va_copy(ap_copy, ap);
  const int len = std::vsnprintf(buf, sizeof(buf), fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0) {
    Runtime_ThrowError(type, nullptr);
    return;
  }

  if (len < static_cast<int>(sizeof(buf))) {
    Runtime_ThrowError(type, buf);
    return;
  }

  std::string dynamic(static_cast<size_t>(len) + 1, '\0');
  (void)std::vsnprintf(dynamic.data(), dynamic.size(), fmt, ap);
  Runtime_ThrowError(type, dynamic.c_str());
}

}  // namespace

void Runtime_ThrowError(ExceptionType type, const char* message) {
  Handle<PyString> type_name = GetExceptionStringHandle(type);
  if (message == nullptr) {
    ThrowNewException(type_name, Handle<PyString>::null());
    return;
  }
  ThrowNewException(type_name, PyString::NewInstance(message));
}

void Runtime_ThrowErrorf(ExceptionType type, const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(type, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(type, fmt, ap);
  va_end(ap);
}

// --- 旧 API 兼容层实现 (Variadic functions) ---
// 非 Variadic 版本已在头文件中内联实现。

void Runtime_ThrowTypeErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kTypeError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kTypeError, fmt, ap);
  va_end(ap);
}

void Runtime_ThrowRuntimeErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kRuntimeError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kRuntimeError, fmt, ap);
  va_end(ap);
}

void Runtime_ThrowValueErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kValueError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kValueError, fmt, ap);
  va_end(ap);
}

void Runtime_ThrowIndexErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kIndexError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kIndexError, fmt, ap);
  va_end(ap);
}

void Runtime_ThrowKeyErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kKeyError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kKeyError, fmt, ap);
  va_end(ap);
}

void Runtime_ThrowNameErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(ExceptionType::kNameError, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(ExceptionType::kNameError, fmt, ap);
  va_end(ap);
}

Handle<PyString> Runtime_FormatPendingExceptionForStderr() {
  EscapableHandleScope scope;

  auto* state = Isolate::Current()->exception_state();
  if (!state->HasPendingException()) {
    return scope.Escape(PyString::NewInstance(""));
  }

  Handle<PyObject> exception = state->pending_exception();
  Handle<PyString> type_name = PyObject::GetKlass(exception)->name();

  Handle<PyString> message = Handle<PyString>::null();
  Handle<PyDict> properties = PyObject::GetProperties(exception);
  if (!properties.is_null()) {
    Handle<PyObject> msg_obj = properties->Get(ST(message));
    if (!msg_obj.is_null() && IsPyString(*msg_obj)) {
      message = Handle<PyString>::cast(msg_obj);
    }
  }

  if (message.is_null() || message->IsEmpty()) {
    return scope.Escape(type_name);
  }

  std::string formatted;
  formatted.reserve(static_cast<size_t>(type_name->length()) + 2 +
                    static_cast<size_t>(message->length()));
  formatted.append(type_name->buffer(),
                   static_cast<size_t>(type_name->length()));
  formatted.append(": ");
  formatted.append(message->buffer(), static_cast<size_t>(message->length()));
  return scope.Escape(PyString::NewInstance(
      formatted.c_str(), static_cast<int64_t>(formatted.size())));
}

}  // namespace saauso::internal
