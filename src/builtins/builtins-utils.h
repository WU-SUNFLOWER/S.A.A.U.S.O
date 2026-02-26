// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_UTILS_H_
#define SAAUSO_BUILTINS_BUILTINS_UTILS_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

// 特别提醒：
// 如果需要Python内建函数对应的C++函数签名声明，
// 请使用src/objects/py-function.h当中的NativeFuncPointer！！！

class PyObject;
class PyTuple;
class PyDict;

#define BUILTIN_FUNC_NAME(name) Builtin_##name

#define BUILTIN(name)                            \
  MaybeHandle<PyObject> BUILTIN_FUNC_NAME(name)( \
      Handle<PyObject> host, Handle<PyTuple> args, Handle<PyDict> kwargs)

#define DECL_BUILTIN_METHOD(name, _) static BUILTIN(name);

#define BUILTIN_METHOD(type, name)                     \
  MaybeHandle<PyObject> type::BUILTIN_FUNC_NAME(name)( \
      Handle<PyObject> self, Handle<PyTuple> args, Handle<PyDict> kwargs)

#define INSTALL_BUILTIN_METHOD(func_name, method_name)                      \
  do {                                                                      \
    auto prop_name = PyString::NewInstance(method_name);                    \
    (void)PyDict::Put(                                                      \
        target, prop_name,                                                  \
        PyFunction::NewInstance(&BUILTIN_FUNC_NAME(func_name), prop_name)); \
  } while (0);

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_UTILS_H_
