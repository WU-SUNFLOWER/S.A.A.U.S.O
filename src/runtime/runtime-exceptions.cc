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

MaybeHandle<PyObject> Runtime_NewExceptionInstance(
    Handle<PyString> exception_type_name,
    Handle<PyString> message_or_null) {
  EscapableHandleScope scope;
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

  return scope.Escape(exception);
}

void Runtime_ThrowNewException(Handle<PyString> exception_type_name,
                               Handle<PyString> message_or_null) {
  auto* state = Isolate::Current()->exception_state();
  if (state->HasPendingException()) {
    return;
  }

  auto* isolate = Isolate::Current();
  Handle<PyObject> exception;

  ASSIGN_RETURN_ON_EXCEPTION_VALUE(
      isolate, exception,
      Runtime_NewExceptionInstance(exception_type_name, message_or_null), );

  state->Throw(*exception);
}

void Runtime_ThrowTypeError(const char* message) {
  if (message == nullptr) {
    Runtime_ThrowNewException(ST(type_err), Handle<PyString>::null());
    return;
  }
  Runtime_ThrowNewException(ST(type_err), PyString::NewInstance(message));
}

void Runtime_ThrowRuntimeError(const char* message) {
  if (message == nullptr) {
    Runtime_ThrowNewException(ST(runtime_err), Handle<PyString>::null());
    return;
  }
  Runtime_ThrowNewException(ST(runtime_err), PyString::NewInstance(message));
}

void Runtime_ThrowTypeErrorf(const char* fmt, ...) {
  if (fmt == nullptr) {
    Runtime_ThrowTypeError(nullptr);
    return;
  }

  va_list ap;
  va_start(ap, fmt);

  char buf[256];
  const int len = std::vsnprintf(buf, sizeof(buf), fmt, ap);

  va_end(ap);

  if (len < 0) {
    Runtime_ThrowTypeError(nullptr);
    return;
  }

  if (len < static_cast<int>(sizeof(buf))) {
    Runtime_ThrowTypeError(buf);
    return;
  }

  std::string dynamic(static_cast<size_t>(len) + 1, '\0');

  va_start(ap, fmt);
  (void)std::vsnprintf(dynamic.data(), dynamic.size(), fmt, ap);
  va_end(ap);

  Runtime_ThrowTypeError(dynamic.c_str());
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
