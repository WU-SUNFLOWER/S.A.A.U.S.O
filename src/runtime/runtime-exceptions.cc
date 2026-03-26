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
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

constexpr size_t kFormattedErrorBufferSize = 256;

MaybeHandle<PyString> MessageFromArgsTuple(Handle<PyTuple> exception_args) {
  if (exception_args.is_null() || exception_args->length() == 0) {
    return PyString::NewInstance("");
  }
  if (exception_args->length() == 1 && IsPyString(exception_args->Get(0))) {
    return Handle<PyString>::cast(exception_args->Get(0));
  }
  return PyString::NewInstance("");
}

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
      Runtime_NewExceptionInstance(isolate, exception_type_name,
                                   message_or_null));

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
    Isolate* isolate,
    Handle<PyString> exception_type_name,
    Handle<PyString> message_or_null) {
  EscapableHandleScope scope;

  Handle<PyDict> builtins = handle(isolate->builtins());

  Handle<PyObject> exception_type;
  RETURN_ON_EXCEPTION(isolate,
                      builtins->Get(exception_type_name, exception_type));
  assert(!exception_type.is_null());

  Handle<PyTuple> init_args = Handle<PyTuple>::null();
  if (!message_or_null.is_null()) {
    init_args = PyTuple::NewInstance(1);
    init_args->SetInternal(0, message_or_null);
  }

  Handle<PyObject> exception = Handle<PyObject>::null();
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, exception,
      Runtime_NewObject(isolate, Handle<PyTypeObject>::cast(exception_type),
                        init_args, Handle<PyObject>::null()));

  if (!message_or_null.is_null()) {
    Handle<PyDict> properties = PyObject::GetProperties(exception);
    if (!properties.is_null()) {
      RETURN_ON_EXCEPTION_VALUE(
          isolate, PyDict::Put(properties, ST(message), message_or_null),
          kNullMaybeHandle);
    }
  }

  return scope.Escape(exception);
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

MaybeHandle<PyString> Runtime_FormatPendingExceptionForStderr(
    Isolate* isolate) {
  EscapableHandleScope scope;

  auto* state = isolate->exception_state();
  if (!state->HasPendingException()) {
    return scope.Escape(PyString::NewInstance(""));
  }

  Handle<PyObject> exception = state->pending_exception();
  Handle<PyString> type_name = PyObject::GetKlass(exception)->name();

  Handle<PyString> message = Handle<PyString>::null();
  Handle<PyDict> properties = PyObject::GetProperties(exception);
  if (!properties.is_null()) {
    Handle<PyObject> args_obj;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                               properties->Get(ST(args), args_obj));
    if (found && IsPyTuple(args_obj)) {
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, message,
          MessageFromArgsTuple(Handle<PyTuple>::cast(args_obj)));
    }

    Handle<PyObject> msg_obj;
    found = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                               properties->Get(ST(message), msg_obj));
    if ((message.is_null() || message->IsEmpty()) && found &&
        IsPyString(msg_obj)) {
      message = Handle<PyString>::cast(msg_obj);
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

  Handle<PyDict> builtins = handle(isolate->builtins());
  Handle<PyObject> stop_iter_type;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, found, builtins->Get(ST(stop_iter), stop_iter_type), kNullMaybe);
  assert(found);
  assert(!stop_iter_type.is_null());

  bool is_stop_iteration = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_stop_iteration,
      Runtime_IsInstanceOfTypeObject(
          isolate, pending, Handle<PyTypeObject>::cast(stop_iter_type)));

  if (!is_stop_iteration) {
    return Maybe<bool>(false);
  }

  state->Clear();
  return Maybe<bool>(true);
}

}  // namespace saauso::internal
