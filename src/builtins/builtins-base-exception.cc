// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-base-exception.h"

#include "src/execution/exception-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

void BaseExceptionMethods::Install(Handle<PyDict> target) {
  BASE_EXCEPTION_BUILTINS(INSTALL_BUILTIN_METHOD);
}

////////////////////////////////////////////////////////////////////////

namespace {

MaybeHandle<PyObject> BaseExceptionStrImpl(Handle<PyObject> self,
                                           Handle<PyTuple> args,
                                           Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() takes no keyword arguments");
    return Handle<PyObject>::null();
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() takes no arguments");
    return Handle<PyObject>::null();
  }

  if (self.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() expects a non-null receiver");
    return Handle<PyObject>::null();
  }

  Handle<PyDict> properties = PyObject::GetProperties(self);
  if (!properties.is_null()) {
    Handle<PyObject> message;
    bool found = false;
    if (!properties->Get(ST(message), message).To(&found)) {
      return kNullMaybeHandle;
    }
    if (found && IsPyString(message)) {
      return scope.Escape(Handle<PyString>::cast(message));
    }
  }

  return scope.Escape(PyString::NewInstance(""));
}

}  // namespace

////////////////////////////////////////////////////////////////////////

// BaseException.__str__ 的最小实现：返回 message 字段。
// 该实现用于提升 MVP 阶段的可用性，使用户能够通过 str(e) 获取错误原因。
BUILTIN_METHOD(BaseExceptionMethods, Str) {
  return BaseExceptionStrImpl(self, args, kwargs);
}

// BaseException.__repr__ 的最小实现：返回 "<TypeName: message>"（message
// 为空则省略）。 该实现主要用于调试与单测断言，MVP 阶段不追求完全对齐 CPython
// repr。
BUILTIN_METHOD(BaseExceptionMethods, Repr) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no keyword arguments");
    return Handle<PyObject>::null();
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no arguments");
    return Handle<PyObject>::null();
  }

  if (self.is_null()) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__repr__() expects a non-null receiver");
    return Handle<PyObject>::null();
  }

  Handle<PyString> type_name = PyObject::GetKlass(self)->name();
  Handle<PyString> message;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), message,
                             BaseExceptionStrImpl(self, args, kwargs));

  Handle<PyString> result = PyString::NewInstance("<");
  if (message.is_null() || message->IsEmpty()) {
    result = PyString::Append(result, type_name);
  } else {
    result = PyString::Append(result, type_name);
    result = PyString::Append(result, PyString::NewInstance(": "));
    result = PyString::Append(result, message);
  }
  result = PyString::Append(result, PyString::NewInstance(">"));

  return scope.Escape(result);
}

}  // namespace saauso::internal
