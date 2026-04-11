// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-exceptions.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "src/execution/exception-roots.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-reflection.h"

namespace saauso::internal {

namespace {

constexpr size_t kFormattedErrorBufferSize = 256;

Handle<PyTuple> ReadExceptionArgsFromLayout(Isolate* isolate,
                                            Handle<PyObject> exception) {
  if (!IsHeapObject(*exception)) {
    return Handle<PyTuple>::null();
  }

  Tagged<Klass> klass = PyObject::GetHeapKlassUnchecked(*exception);
  if (klass->native_layout_kind() != NativeLayoutKind::kBaseException) {
    return Handle<PyTuple>::null();
  }
  return PyBaseException::cast(*exception)->args(isolate);
}

void ThrowNewException(Isolate* isolate,
                       ExceptionType type,
                       Handle<PyString> message_or_null) {
  auto* state = isolate->exception_state();
  if (state->HasPendingException()) {
    return;
  }

  Handle<PyObject> exception =
      isolate->factory()->NewExceptionFromMessage(type, message_or_null);
  state->Throw(*exception);
}

// 将 va_list 格式化为字符串并抛出指定类型的异常。
void ThrowFormattedError(Isolate* isolate,
                         ExceptionType type,
                         const char* fmt,
                         va_list ap) {
  char buf[kFormattedErrorBufferSize];
  va_list ap_copy;
  va_copy(ap_copy, ap);
  const int len = std::vsnprintf(buf, sizeof(buf), fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0) {
    Runtime_ThrowError(isolate, type, nullptr);
    return;
  }

  if (len < static_cast<int>(sizeof(buf))) {
    Runtime_ThrowError(isolate, type, buf);
    return;
  }

  // 在超长消息时回退到使用std::string缓冲。
  std::string dynamic(static_cast<size_t>(len) + 1, '\0');
  std::vsnprintf(dynamic.data(), dynamic.size(), fmt, ap);
  Runtime_ThrowError(isolate, type, dynamic.c_str());
}

}  // namespace

//////////////////////////////////////////////////////////////////////////

void Runtime_ThrowError(Isolate* isolate,
                        ExceptionType type,
                        const char* message) {
  Handle<PyString> wrapped_message = message != nullptr
                                         ? PyString::New(isolate, message)
                                         : Handle<PyString>::null();
  ThrowNewException(isolate, type, wrapped_message);
}

void Runtime_ThrowErrorf(Isolate* isolate,
                         ExceptionType type,
                         const char* fmt,
                         ...) {
  if (fmt == nullptr) {
    Runtime_ThrowError(isolate, type, nullptr);
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  ThrowFormattedError(isolate, type, fmt, ap);
  va_end(ap);
}

MaybeHandle<PyString> Runtime_FormatPendingExceptionForStderr(
    Isolate* isolate) {
  EscapableHandleScope scope(isolate);

  auto* state = isolate->exception_state();
  if (!state->HasPendingException()) {
    return scope.Escape(PyString::New(isolate, ""));
  }

  auto exception =
      Handle<PyBaseException>::cast(state->pending_exception(isolate));
  Handle<PyString> type_name = PyObject::GetTypeName(exception, isolate);

  Handle<PyTuple> exception_args =
      ReadExceptionArgsFromLayout(isolate, exception);
  if (exception_args.is_null()) {
    return scope.Escape(type_name);
  }
  Handle<PyString> message;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, message,
      Runtime_ParseExceptionMessageFromArgs(isolate, exception));

  if (message.is_null() || message->IsEmpty()) {
    return scope.Escape(type_name);
  }

  Handle<PyString> formatted = type_name;
  formatted =
      PyString::Append(formatted, PyString::New(isolate, ": "), isolate);
  formatted = PyString::Append(formatted, message, isolate);

  return scope.Escape(formatted);
}

Maybe<bool> Runtime_ConsumePendingStopIterationIfSet(Isolate* isolate) {
  auto* state = isolate->exception_state();
  if (!state->HasPendingException()) {
    return Maybe<bool>(false);
  }

  HandleScope scope(isolate);
  Handle<PyObject> pending = state->pending_exception(isolate);
  Handle<PyTypeObject> stop_iter_type =
      isolate->exception_roots()->Get(ExceptionType::kStopIteration);

  bool is_stop_iteration = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_stop_iteration,
      Runtime_IsInstanceOfTypeObject(isolate, pending, stop_iter_type));

  if (!is_stop_iteration) {
    return Maybe<bool>(false);
  }

  state->Clear();
  return Maybe<bool>(true);
}

MaybeHandle<PyString> Runtime_ParseExceptionMessageFromArgs(
    Isolate* isolate,
    Handle<PyBaseException> exception) {
  Handle<PyTuple> exception_args = exception->args(isolate);

  if (exception_args.is_null() || exception_args->length() == 0) {
    return PyString::New(isolate, "");
  }

  if (exception_args->length() == 1) {
    Handle<PyObject> message_obj = exception_args->Get(0, isolate);
    Handle<PyObject> message_as_str;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, message_as_str,
                               PyObject::Str(isolate, message_obj));
    return Handle<PyString>::cast(message_as_str);
  }

  Handle<PyObject> args_as_str;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, args_as_str,
                             PyObject::Str(isolate, exception_args));
  return Handle<PyString>::cast(args_as_str);
}

}  // namespace saauso::internal
