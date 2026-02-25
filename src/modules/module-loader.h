// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_LOADER_H_
#define SAAUSO_MODULES_MODULE_LOADER_H_

#include <cstddef>
#include <string>

#include "src/handles/maybe-handles.h"
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

  // 失败时设置 pending exception 并返回空 MaybeHandle。
  MaybeHandle<PyModule> LoadModulePart(Handle<PyString> fullname,
                                       Handle<PyList> search_path_list);

 private:
  // 成功返回模块；未找到或异常时返回None
  MaybeHandle<PyObject> LoadModulePartOrNoneImpl(
      Handle<PyString> fullname,
      Handle<PyList> search_path_list);

  MaybeHandle<PyObject> ExecuteModuleOrNoneInternal(Handle<PyString> fullname,
                                                    const ModuleLocation& loc);

  Handle<PyObject> LoadAsBuiltinModuleOrNone(Handle<PyString> fullname);

  MaybeHandle<PyObject> LoadAsFileModuleOrNone(Handle<PyString> fullname,
                                               Handle<PyList> search_path_list);

  MaybeHandle<PyModule> ExecuteModuleFromSource(Handle<PyString> fullname,
                                                const ModuleLocation& loc,
                                                Handle<PyString> source);

  MaybeHandle<PyModule> ExecuteModuleFromPyc(Handle<PyString> fullname,
                                             const ModuleLocation& loc);

  MaybeHandle<PyObject> InitializeModuleDict(Handle<PyModule> module,
                                             Handle<PyString> fullname,
                                             const ModuleLocation& loc);

  Isolate* isolate_{nullptr};
  ModuleFinder* finder_{nullptr};
  ModuleManager* manager_{nullptr};
  BuiltinModuleRegistry* builtin_registry_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_LOADER_H_
