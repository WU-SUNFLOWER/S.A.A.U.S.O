// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-function.h"

#include <string>

#include "src/execution/exception-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-reflection.h"

namespace saauso::internal {

MaybeHandle<PyString> Runtime_NewFunctionRepr(Handle<PyFunction> func) {
  EscapableHandleScope scope;
  std::string repr;
  if (IsNativePyFunction(func)) {
    repr = "<built-in function ";
    repr.append(func->func_name()->ToStdString());
    repr.push_back('>');
    return scope.Escape(PyString::FromStdString(repr));
  }

  repr = "<function ";
  repr.append(func->func_name()->ToStdString());
  char addr[32];
  std::snprintf(addr, sizeof(addr), " at 0x%p>",
                reinterpret_cast<void*>((*func).ptr()));
  repr.append(addr);

  return scope.Escape(PyString::FromStdString(repr));
}

MaybeHandle<PyString> Runtime_NewMethodObjectRepr(Handle<MethodObject> method) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "<method object at %p>",
                reinterpret_cast<void*>((*method).ptr()));
  return PyString::NewInstance(buffer);
}

Maybe<void> Runtime_NormalizeNativeMethodCall(Isolate* isolate,
                                              Handle<PyFunction> func,
                                              Handle<PyObject>& receiver,
                                              Handle<PyTuple>& args) {
  // Fast Path: 非instance method的 native 函数，不需要任何预处理，直接返回
  if (!func->is_native_instance_method()) {
    return JustVoid();
  }

  Handle<PyTypeObject> owner_type = func->native_owner_type();

  // Fast Path: 对于已知receiver显式方法调用，直接返回
  if (!receiver.is_null()) [[likely]] {
#if defined(_DEBUG)
    bool is_valid_receiver = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, is_valid_receiver,
                               Runtime_IsSubtype(PyObject::GetKlass(receiver),
                                                 owner_type->own_klass()));
    assert(is_valid_receiver);
#endif  // defined(_DEBUG)

    return JustVoid();
  }

  // Slow Path:
  // 类似于`str.upper(s)`、`list.append(l,1)`
  // 的间接方法调用，需要手工从 args 中解包出 receiver
  if (receiver.is_null()) {
    int64_t argc = args.is_null() ? 0 : args->length();
    if (argc == 0) [[unlikely]] {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "unbound method %s.%s() needs an argument",
                          owner_type->own_klass()->name()->buffer(),
                          func->func_name()->buffer());
      return kNullMaybe;
    }

    receiver = args->Get(0);
    if (argc == 1) {
      args = Handle<PyTuple>::null();
    } else {
      Handle<PyTuple> tail = PyTuple::NewInstance(argc - 1);
      for (int64_t i = 1; i < argc; ++i) {
        tail->SetInternal(i - 1, *args->Get(i));
      }
      args = tail;
    }
  }

  bool is_valid_receiver = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_valid_receiver,
      Runtime_IsSubtype(PyObject::GetKlass(receiver), owner_type->own_klass()));
  if (!is_valid_receiver) [[unlikely]] {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "descriptor '%s' for '%s' objects doesn't apply to a '%s' object",
        func->func_name()->buffer(), owner_type->own_klass()->name()->buffer(),
        PyObject::GetKlass(receiver)->name()->buffer());
    return kNullMaybe;
  }

  return JustVoid();
}

MaybeHandle<PyObject> Runtime_CallNativePyFunction(Isolate* isolate,
                                                   Handle<PyFunction> func,
                                                   Handle<PyObject> receiver,
                                                   Handle<PyTuple> args,
                                                   Handle<PyDict> kwargs) {
  assert(IsNativePyFunction(func));

  RETURN_ON_EXCEPTION(isolate, Runtime_NormalizeNativeMethodCall(
                                   isolate, func, receiver, args));

  NativeFuncPointer native_func_ptr = func->native_func();
  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, result,
                             native_func_ptr(isolate, receiver, args, kwargs));

  return result;
}

}  // namespace saauso::internal
