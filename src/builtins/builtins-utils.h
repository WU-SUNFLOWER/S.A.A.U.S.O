// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_BUILTINS_BUILTINS_UTILS_H_
#define SAAUSO_BUILTINS_BUILTINS_UTILS_H_

#include "src/execution/exception-utils.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-string.h"
#include "src/objects/templates.h"

namespace saauso::internal {

// 特别提醒：
// 如果需要Python内建函数对应的C++函数签名声明，
// 请使用src/objects/native-function-helpers.h当中的NativeFuncPointer！！！

class PyObject;
class PyTuple;

#define BUILTIN_FUNC_NAME(name) Builtin_##name

#define BUILTIN(name)                            \
  MaybeHandle<PyObject> BUILTIN_FUNC_NAME(name)( \
      Handle<PyObject> host, Handle<PyTuple> args, Handle<PyDict> kwargs)

#define DECL_BUILTIN_METHOD(name, _) static BUILTIN(name);

#define BUILTIN_METHOD(type, name)                     \
  MaybeHandle<PyObject> type::BUILTIN_FUNC_NAME(name)( \
      Handle<PyObject> self, Handle<PyTuple> args, Handle<PyDict> kwargs)

#define INSTALL_BUILTIN_METHOD(func_name, method_name)                         \
  do {                                                                         \
    auto prop_name = PyString::NewInstance(method_name);                       \
    auto func_template =                                                       \
        FunctionTemplateInfo(&BUILTIN_FUNC_NAME(func_name), prop_name);        \
    Handle<PyFunction> func_object;                                            \
    ASSIGN_RETURN_ON_EXCEPTION(                                                \
        isolate, func_object,                                                  \
        isolate->factory()->NewPyFunctionWithTemplate(func_template));         \
    RETURN_ON_EXCEPTION(isolate, PyDict::Put(target, prop_name, func_object)); \
  } while (0);

}  // namespace saauso::internal

#endif  // SAAUSO_BUILTINS_BUILTINS_UTILS_H_
