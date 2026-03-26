// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/builtins-py-tuple-methods.h"

#include <algorithm>
#include <cinttypes>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-conversions.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"

namespace saauso::internal {

Maybe<void> PyTupleBuiltinMethods::Install(Isolate* isolate,
                                           Handle<PyDict> target,
                                           Handle<PyTypeObject> owner_type) {
  // INSTALL_BUILTIN_METHOD宏用于显式捕获局部变量isolate和target
#define INSTALL_BUILTIN_METHOD(cpp_func_name, method_name, access_flag)    \
  INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name, method_name, \
                              access_flag, owner_type)

  PY_TUPLE_BUILTINS(INSTALL_BUILTIN_METHOD);
#undef INSTALL_BUILTIN_METHOD

  return JustVoid();
}

////////////////////////////////////////////////////////////////////////

BUILTIN_METHOD(PyTupleBuiltinMethods, Repr) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "tuple.__repr__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Repr(isolate, self);
}

BUILTIN_METHOD(PyTupleBuiltinMethods, Str) {
  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc != 0) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "tuple.__str__() takes no arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  return PyObject::Str(isolate, self);
}

BUILTIN_METHOD(PyTupleBuiltinMethods, Index) {
  EscapableHandleScope scope;
  auto tuple = Handle<PyTuple>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "tuple.index() takes no keyword arguments");
    return kNullMaybeHandle;
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "tuple.index() takes at least 1 argument (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }
  if (argc > 3) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "tuple.index() takes at most 3 arguments (%" PRId64 " given)", argc);
    return kNullMaybeHandle;
  }

  auto target = args->Get(0);
  int64_t length = tuple->length();
  int64_t begin = 0;
  int64_t end = length;

  if (argc >= 2) {
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, begin, Runtime_DecodeIntLike(isolate, args->GetTagged(1)));
  }
  if (argc >= 3) {
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, end, Runtime_DecodeIntLike(isolate, args->GetTagged(2)));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyTuple::kNotFound;
  if (begin <= end) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate, result,
                               tuple->IndexOf(target, begin, end));
  }
  if (result == PyTuple::kNotFound) {
    Runtime_ThrowError(isolate, ExceptionType::kValueError,
                       "tuple.index(x): x not in tuple");
    return kNullMaybeHandle;
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

}  // namespace saauso::internal
