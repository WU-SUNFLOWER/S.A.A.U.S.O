// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_UTILS_H_
#define SAAUSO_BUILTINS_BUILTINS_UTILS_H_

#include "src/execution/exception-utils.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/native-function-helpers.h"

namespace saauso::internal {

// 特别提醒：
// 如果需要Python内建函数对应的C++函数签名声明，
// 请使用src/objects/native-function-helpers.h当中的NativeFuncPointer！！！

class Isolate;
class PyObject;
class PyTuple;
class PyTypeObject;

#define BUILTIN_FUNC_NAME(name) Builtin_##name

#define BUILTIN(name)                                                     \
  MaybeHandle<PyObject> BUILTIN_FUNC_NAME(name)(                          \
      Isolate * isolate, Handle<PyObject> receiver, Handle<PyTuple> args, \
      Handle<PyDict> kwargs)

#define DECL_BUILTIN_METHOD(name, ignore1, ignore2) static BUILTIN(name);

#define BUILTIN_METHOD(type, name)                                    \
  MaybeHandle<PyObject> type::BUILTIN_FUNC_NAME(name)(                \
      Isolate * isolate, Handle<PyObject> self, Handle<PyTuple> args, \
      Handle<PyDict> kwargs)

Maybe<void> InstallBuiltinMethodImpl(
    Isolate* isolate,
    Handle<PyDict> target,
    NativeFuncPointer cpp_func,
    const char* method_name,
    NativeFuncAccessFlag access_flag,
    Handle<PyTypeObject> owner_type = Handle<PyTypeObject>::null());

#define INSTALL_BUILTIN_METHOD_IMPL(isolate, target, cpp_func_name,       \
                                    method_name, access_flag, owner_type) \
  RETURN_ON_EXCEPTION(                                                    \
      isolate,                                                            \
      InstallBuiltinMethodImpl(                                           \
          (isolate), (target), &BUILTIN_FUNC_NAME(cpp_func_name),         \
          (method_name), (NativeFuncAccessFlag::access_flag), (owner_type)));

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_UTILS_H_
