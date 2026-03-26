// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-base-exception-methods.h"

#include "src/execution/exception-utils.h"
#include "src/objects/py-base-exception-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"

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

BUILTIN_METHOD(BaseExceptionMethods, Init) {
  return PyBaseExceptionKlass::GetInstance(isolate)->InitInstance(isolate, self,
                                                                  args, kwargs);
}

BUILTIN_METHOD(BaseExceptionMethods, Str) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "BaseException.__str__() takes no keyword arguments");
    return kNullMaybeHandle;
  }
  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "BaseException.__str__() takes no arguments");
    return kNullMaybeHandle;
  }
  return PyObject::Str(isolate, self);
}

BUILTIN_METHOD(BaseExceptionMethods, Repr) {
  if (!kwargs.is_null() && kwargs->occupied() != 0) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  if (!args.is_null() && args->length() != 0) [[unlikely]] {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "BaseException.__repr__() takes no arguments");
    return kNullMaybeHandle;
  }
  return PyObject::Repr(isolate, self);
}

}  // namespace saauso::internal
