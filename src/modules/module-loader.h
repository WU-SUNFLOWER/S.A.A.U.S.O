// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_EXECUTOR_H_
#define SAAUSO_MODULES_MODULE_EXECUTOR_H_

#include <cstddef>
#include <string>
#include <vector>

#include "src/modules/builtin-module.h"

namespace saauso::internal {

class Isolate;
class BuiltinModuleRegistry;
class ModuleFinder;
class ModuleManager;
class PyDict;
class PyObject;
class PyString;
class ModuleLocation;

class ModuleLoader final {
 public:
  ModuleLoader(Isolate* isolate,
               ModuleFinder* finder,
               ModuleManager* manager,
               BuiltinModuleRegistry* builtin_registry);
  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;
  ~ModuleLoader() = default;

  Handle<PyObject> LoadModulePart(Handle<PyString> fullname,
                                  const std::vector<std::string>& search_paths);

 private:
  Handle<PyModule> ExecuteModuleImpl(Handle<PyString> fullname,
                                     const ModuleLocation& loc,
                                     const std::string& source);

  Isolate* isolate_{nullptr};
  ModuleFinder* finder_{nullptr};
  ModuleManager* manager_{nullptr};
  BuiltinModuleRegistry* builtin_registry_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_EXECUTOR_H_
