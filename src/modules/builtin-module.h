// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_BUILTIN_MODULE_H_
#define SAAUSO_MODULES_BUILTIN_MODULE_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
class ModuleManager;
class PyModule;

using BuiltinModuleInitFunc = Handle<PyModule> (*)(Isolate* isolate,
                                                   ModuleManager* manager);

#define BUILTIN_MODULE_INIT_FUNC(module_name, func_name) \
  Handle<PyModule> func_name(Isolate* isolate, ModuleManager* manager)

#define DECL_BUILTIN_MODULE_INIT_FUNC(module_name, func_name) \
  BUILTIN_MODULE_INIT_FUNC(module_name, func_name);

#define BUILTIN_MODULE_LIST(V)  \
  V("sys", InitSysModule)       \
  V("math", InitMathModule)     \
  V("random", InitRandomModule) \
  V("time", InitTimeModule)

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_H_
