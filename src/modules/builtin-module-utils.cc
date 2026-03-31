// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/builtin-module-utils.h"

#include "src/execution/exception-utils.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/templates.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

MaybeHandle<PyModule> BuiltinModuleUtils::NewBuiltinModule(
    Isolate* isolate,
    const char* module_name,
    const char* package_name) {
  EscapableHandleScope scope(isolate);

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, module,
                             isolate->factory()->NewPyModule());

  RETURN_ON_EXCEPTION(isolate, BuiltinModuleUtils::InitializeBuiltinModule(
                                   isolate, module, module_name, package_name));
  return scope.Escape(module);
}

void BuiltinModuleUtils::ThrowNoKeywordArgsError(Isolate* isolate,
                                                 const char* module_name,
                                                 const char* func_name) {
  Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                      "%s.%s() takes no keyword arguments", module_name,
                      func_name);
}

Maybe<void> BuiltinModuleUtils::InitializeBuiltinModule(
    Isolate* isolate,
    Handle<PyModule> module,
    const char* module_name,
    const char* package_name) {
  Handle<PyDict> module_dict = PyObject::GetProperties(module, isolate);

  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(module_dict, ST(name, isolate),
                           PyString::New(isolate, module_name), isolate));
  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(module_dict, ST(package, isolate),
                           PyString::New(isolate, package_name), isolate));

  return JustVoid();
}

Maybe<void> BuiltinModuleUtils::InstallBuiltinModuleFunc(
    Isolate* isolate,
    Handle<PyDict> module_dict,
    const char* name,
    NativeFuncPointer func) {
  // 这里不直接复用 builtins-utils 的安装入口，是为了让 modules 层保持
  // “无 owner_type/access_flag
  // 语义”的最小接口，避免模块函数安装与类型方法安装耦合。
  HandleScope scope(isolate);

  Handle<PyString> py_name = PyString::New(isolate, name);
  FunctionTemplateInfo function_template(isolate, func, py_name);

  Handle<PyFunction> function_object;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, function_object,
      isolate->factory()->NewPyFunctionWithTemplate(function_template));

  RETURN_ON_EXCEPTION(
      isolate, PyDict::Put(module_dict, py_name, function_object, isolate));

  return JustVoid();
}

Maybe<void> BuiltinModuleUtils::InstallBuiltinModuleFuncsFromSpec(
    Isolate* isolate,
    Handle<PyModule> module,
    const BuiltinModuleFuncSpec* specs,
    int64_t spec_count) {
  Handle<PyDict> module_dict = PyObject::GetProperties(module, isolate);
  for (int64_t i = 0; i < spec_count; ++i) {
    RETURN_ON_EXCEPTION(
        isolate, BuiltinModuleUtils::InstallBuiltinModuleFunc(
                     isolate, module_dict, specs[i].name, specs[i].func));
  }

  return JustVoid();
}

}  // namespace saauso::internal
