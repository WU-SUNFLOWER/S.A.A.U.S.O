// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-base-exception-methods.h"

#include "src/execution/exception-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Maybe<void> BaseExceptionMethods::Install(Isolate* isolate,
                                          Handle<PyDict> target,
                                          Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  BASE_EXCEPTION_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

namespace {

MaybeHandle<PyString> MessageFromArgsTuple(Handle<PyTuple> exception_args) {
  if (exception_args.is_null() || exception_args->length() == 0) {
    return PyString::NewInstance("");
  }
  if (exception_args->length() == 1 && IsPyString(exception_args->Get(0))) {
    return Handle<PyString>::cast(exception_args->Get(0));
  }
  return PyString::NewInstance("");
}

MaybeHandle<PyTuple> ReadExceptionArgs(Isolate* isolate,
                                       Handle<PyObject> self) {
  Handle<PyDict> properties = PyObject::GetProperties(self);
  if (properties.is_null()) {
    return Handle<PyTuple>::null();
  }

  Handle<PyObject> args_obj;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             properties->Get(ST(args), args_obj));
  if (!found || !IsPyTuple(args_obj)) {
    return Handle<PyTuple>::null();
  }
  return Handle<PyTuple>::cast(args_obj);
}

MaybeHandle<PyObject> BaseExceptionStrImpl(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyTuple> args,
                                           Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__str__() takes no arguments");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> exception_args;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, exception_args,
                             ReadExceptionArgs(isolate, self));
  if (!exception_args.is_null()) {
    Handle<PyString> args_message;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, args_message,
                               MessageFromArgsTuple(exception_args));
    return scope.Escape(args_message);
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

BUILTIN_METHOD(BaseExceptionMethods, Init) {
  return PyBaseExceptionKlass::GetInstance()->InitInstance(isolate, self, args,
                                                           kwargs);
}

// BaseException.__str__ 的最小实现：返回 message 字段。
// 该实现用于提升 MVP 阶段的可用性，使用户能够通过 str(e) 获取错误原因。
BUILTIN_METHOD(BaseExceptionMethods, Str) {
  return BaseExceptionStrImpl(isolate, self, args, kwargs);
}

// BaseException.__repr__ 的最小实现：返回 "<TypeName: message>"（message
// 为空则省略）。 该实现主要用于调试与单测断言，MVP 阶段不追求完全对齐 CPython
// repr。
BUILTIN_METHOD(BaseExceptionMethods, Repr) {
  EscapableHandleScope scope;

  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no arguments");
    return kNullMaybeHandle;
  }

  Handle<PyString> type_name = PyObject::GetKlass(self)->name();
  Handle<PyString> message;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, message,
                             BaseExceptionStrImpl(isolate, self, args, kwargs));

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
