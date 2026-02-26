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

constexpr size_t kFormattedErrorBufferSize = 256;

void ThrowNewException(Handle<PyString> exception_type_name,
                       Handle<PyString> message_or_null) {
  auto* isolate = Isolate::Current();

  auto* state = isolate->exception_state();
  if (state->HasPendingException()) {
    return;
  }

  Handle<PyObject> exception;
  ASSIGN_RETURN_ON_EXCEPTION_VOID(
      isolate, exception,
      Runtime_NewExceptionInstance(exception_type_name, message_or_null));

  state->Throw(*exception);
}

// 辅助函数：根据枚举类型获取对应的异常类名字符串 Handle
Handle<PyString> GetExceptionStringHandle(ExceptionType type) {
  switch (type) {
#define DEFINE_EXCEPTION_TYPE_CASE(type, string_table_name, _) \
  case ExceptionType::type:                                    \
    return ST(string_table_name);

    EXCEPTION_TYPE_LIST(DEFINE_EXCEPTION_TYPE_CASE)

#undef DEFINE_EXCEPTION_TYPE_CASE
    default:
      // 默认回退到 RuntimeError
      return ST(runtime_err);
  }
}

// 将 va_list 格式化为字符串并抛出指定类型的异常。
void ThrowFormattedError(ExceptionType type, const char* fmt, va_list ap) {
  char buf[kFormattedErrorBufferSize];
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

  // 在超长消息时回退到使用std::string缓冲。
  std::string dynamic(static_cast<size_t>(len) + 1, '\0');
  std::vsnprintf(dynamic.data(), dynamic.size(), fmt, ap);
  Runtime_ThrowError(type, dynamic.c_str());
}

}  // namespace

//////////////////////////////////////////////////////////////////////////

MaybeHandle<PyObject> Runtime_NewExceptionInstance(
    Handle<PyString> exception_type_name,
    Handle<PyString> message_or_null) {
  Handle<PyDict> builtins = Execution::builtins(Isolate::Current());
  Tagged<PyObject> exception_type;
  if (!builtins->GetTaggedMaybe(*exception_type_name).To(&exception_type)) {
    return kNullMaybeHandle;
  }
  assert(!exception_type.is_null());

  Handle<PyObject> exception = Handle<PyObject>::null();
  ASSIGN_RETURN_ON_EXCEPTION(
      Isolate::Current(), exception,
      Runtime_NewObject(Handle<PyTypeObject>::cast(handle(exception_type)),
                        Handle<PyObject>::null(), Handle<PyObject>::null()));

  if (!message_or_null.is_null()) {
    Handle<PyDict> properties = PyObject::GetProperties(exception);
    if (!properties.is_null()) {
      RETURN_ON_EXCEPTION_VALUE(
          Isolate::Current(),
          PyDict::PutMaybe(properties, ST(message), message_or_null),
          kNullMaybeHandle);
    }
  }

  return exception;
}

void Runtime_ThrowError(ExceptionType type, const char* message) {
  Handle<PyString> type_name = GetExceptionStringHandle(type);
  Handle<PyString> wrapped_message = message != nullptr
                                         ? PyString::NewInstance(message)
                                         : Handle<PyString>::null();
  ThrowNewException(type_name, wrapped_message);
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
    Tagged<PyObject> msg_obj;
    if (!properties->GetTaggedMaybe(*ST(message)).To(&msg_obj)) {
      return Handle<PyString>::null();
    }
    if (!msg_obj.is_null() && IsPyString(msg_obj)) {
      message = Handle<PyString>::cast(handle(msg_obj));
    }
  }

  if (message.is_null() || message->IsEmpty()) {
    return scope.Escape(type_name);
  }

  Handle<PyString> formatted = PyString::Clone(type_name);
  formatted = PyString::Append(formatted, PyString::NewInstance(": "));
  formatted = PyString::Append(formatted, message);

  return scope.Escape(formatted);
}

Maybe<bool> Runtime_ConsumePendingStopIterationIfSet(Isolate* isolate) {
  auto* state = isolate->exception_state();
  if (!state->HasPendingException()) {
    return Maybe<bool>(false);
  }

  HandleScope scope;
  Handle<PyObject> pending = state->pending_exception();

  Handle<PyDict> builtins = Execution::builtins(isolate);
  Tagged<PyObject> stop_iter_type;
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(Isolate::Current(), stop_iter_type,
                                   builtins->GetTaggedMaybe(*ST(stop_iter)),
                                   kNullMaybe);

  bool is_stop_iteration = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      Isolate::Current(), is_stop_iteration,
      Runtime_IsInstanceOfTypeObject(
          pending, Handle<PyTypeObject>::cast(handle(stop_iter_type))));

  if (!is_stop_iteration) {
    return Maybe<bool>(false);
  }

  state->Clear();
  return Maybe<bool>(true);
}

}  // namespace saauso::internal
