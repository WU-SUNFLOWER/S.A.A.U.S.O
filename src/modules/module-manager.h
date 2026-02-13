// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_MANAGER_H_
#define SAAUSO_MODULES_MODULE_MANAGER_H_

#include <memory>
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
class ModuleLoader;

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

  Isolate* isolate() const { return isolate_; }
  ModuleLoader* loader() const { return loader_.get(); }

  // 导入模块的统一入口（IMPORT_NAME 会调用此接口）。
  //
  // 参数说明：
  // - name：被导入的模块名。level==0 时不允许为空；level>0 时允许为空（例如
  // - fromlist：IMPORT_NAME 的 fromlist 参数，允许为 null 或空
  // tuple。用于决定返回值语义：
  //   - fromlist 为空且 name 为 dotted-name 时，返回顶层包；
  //   - 否则返回最后导入的模块对象。
  // - level：相对导入层级（0 表示绝对导入）。
  // - globals：当前模块的 globals dict，用于在相对导入时解析
  // __package__/__name__/__path__。
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
  std::unique_ptr<ModuleLoader> loader_;

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
