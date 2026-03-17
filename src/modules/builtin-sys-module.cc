// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-utils.h"
#include "src/handles/handles.h"
#include "src/modules/builtin-module-utils.h"
#include "src/modules/builtin-module.h"
#include "src/modules/module-manager.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

// 初始化 sys 内建模块。
//
// 说明：
// - 该函数仅负责构造模块对象并填充模块字典；
// - sys.modules / sys.path 由 ModuleManager 持有并维护，sys 模块仅把它们暴露给
// Python 层。
BUILTIN_MODULE_INIT_FUNC("sys", InitSysModule) {
  EscapableHandleScope scope;

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, module, BuiltinModuleUtils::NewBuiltinModule(isolate, "sys"));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("modules"),
                                  manager->modules()));

  RETURN_ON_EXCEPTION(
      isolate,
      PyDict::Put(module_dict, PyString::NewInstance("path"), manager->path()));

  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(module_dict, PyString::NewInstance("version"),
                                  PyString::NewInstance("3.12 (saauso mvp)")));

  return scope.Escape(module);
}

}  // namespace saauso::internal
