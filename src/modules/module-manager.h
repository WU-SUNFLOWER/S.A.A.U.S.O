// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_MANAGER_H_
#define SAAUSO_MODULES_MODULE_MANAGER_H_

#include <string>
#include <string_view>
#include <vector>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"

namespace saauso::internal {

class Isolate;
class ObjectVisitor;
class PyDict;
class PyList;
class PyModule;
class PyObject;
class PyString;
class PyTuple;

// ModuleManager 负责管理模块导入过程中的全局状态（sys.modules/sys.path），
// 并提供内建模块的注册与实例化入口。
class ModuleManager final {
 public:
  using BuiltinModuleInitFunc = Handle<PyModule> (*)(Isolate* isolate,
                                                     ModuleManager* manager);

  explicit ModuleManager(Isolate* isolate);
  ModuleManager(const ModuleManager&) = delete;
  ModuleManager& operator=(const ModuleManager&) = delete;
  ~ModuleManager();

  void Iterate(ObjectVisitor* v);

  Handle<PyDict> modules() const;
  Tagged<PyDict> modules_tagged() const;

  Handle<PyList> path() const;
  Tagged<PyList> path_tagged() const;

  // 导入模块的统一入口（IMPORT_NAME 会调用此接口）。
  Handle<PyObject> ImportModule(Handle<PyString> name,
                                Handle<PyTuple> fromlist,
                                int64_t level,
                                Handle<PyDict> globals);

  void RegisterBuiltinModule(std::string_view name, BuiltinModuleInitFunc init);
  BuiltinModuleInitFunc FindBuiltinModule(std::string_view name) const;

 private:
  void InitializeSysState();
  void RegisterBuiltinModules();

  Isolate* isolate_{nullptr};

  Tagged<PyDict> modules_{kNullAddress};
  Tagged<PyList> path_{kNullAddress};

  struct BuiltinModuleEntry {
    std::string name;
    BuiltinModuleInitFunc init{nullptr};
  };
  std::vector<BuiltinModuleEntry> builtin_modules_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_MANAGER_H_

