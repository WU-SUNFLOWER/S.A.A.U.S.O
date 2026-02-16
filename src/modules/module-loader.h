// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_LOADER_H_
#define SAAUSO_MODULES_MODULE_LOADER_H_

#include <cstddef>
#include <string>

#include "src/modules/builtin-module.h"

namespace saauso::internal {

class Isolate;
class BuiltinModuleRegistry;
class ModuleFinder;
class ModuleManager;
class PyList;
class PyDict;
class PyObject;
class PyString;
struct ModuleLocation;

class ModuleLoader final {
 public:
  ModuleLoader(Isolate* isolate,
               ModuleFinder* finder,
               ModuleManager* manager,
               BuiltinModuleRegistry* builtin_registry);
  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;
  ~ModuleLoader() = default;

  Handle<PyModule> LoadModulePart(Handle<PyString> fullname,
                                  Handle<PyList> search_path_list);

 private:
  Handle<PyModule> LoadModulePartImpl(Handle<PyString> fullname,
                                      Handle<PyList> search_path_list);

  Handle<PyModule> ExecuteModuleInternal(Handle<PyString> fullname,
                                         const ModuleLocation& loc);

  Handle<PyModule> LoadAsBuiltinModule(Handle<PyString> fullname);

  Handle<PyModule> LoadAsFileModule(Handle<PyString> fullname,
                                    Handle<PyList> search_path_list);

  Handle<PyModule> ExecuteModuleFromSource(Handle<PyString> fullname,
                                           const ModuleLocation& loc,
                                           Handle<PyString> source);

  Handle<PyModule> ExecuteModuleFromPyc(Handle<PyString> fullname,
                                        const ModuleLocation& loc);

  Isolate* isolate_{nullptr};
  ModuleFinder* finder_{nullptr};
  ModuleManager* manager_{nullptr};
  BuiltinModuleRegistry* builtin_registry_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_LOADER_H_
