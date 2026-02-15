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
#include "src/modules/builtin-module.h"

namespace saauso::internal {

class Isolate;
class ObjectVisitor;
class PyDict;
class PyList;
class PyModule;
class PyObject;
class PyString;
class PyTuple;
class BuiltinModuleRegistry;
class ModuleLoader;
class ModuleFinder;
class ModuleImporter;

// ModuleManager 是虚拟机模块系统的门面：
// - 对解释器提供 ImportModule 接口（导入语义编排）。
// - 持有 sys.modules/sys.path 等全局导入状态。
// - 组合 BuiltinModuleRegistry/ModuleFinder/ModuleLoader 以分离职责边界。
class ModuleManager final {
 public:
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
  ModuleFinder* finder() const { return finder_.get(); }
  ModuleLoader* loader() const { return loader_.get(); }

  // 导入模块的统一入口（IMPORT_NAME 会调用此接口）。
  //
  // 参数说明：
  // - name：被导入的模块名。level==0 时不允许为空；level>0 时允许为空（例如
  //   `from . import x`）。
  // - fromlist：IMPORT_NAME 的 fromlist 参数，允许为 null 或空 tuple。
  //   用于决定返回值语义：
  //   - fromlist 为空且 name 为 dotted-name 时，返回顶层包；
  //   - 否则返回最后导入的模块对象。
  // - level：相对导入层级（0 表示绝对导入）。
  // - globals：当前模块的 globals dict，用于在相对导入时解析
  // __package__/__name__/__path__。
  Handle<PyObject> ImportModule(Handle<PyString> name,
                                Handle<PyTuple> fromlist,
                                int64_t level,
                                Handle<PyDict> globals);

 private:
  friend class ModuleImporter;

  void InitializeSysState();

  Isolate* isolate_{nullptr};
  std::unique_ptr<BuiltinModuleRegistry> builtin_registry_;
  std::unique_ptr<ModuleFinder> finder_;
  std::unique_ptr<ModuleLoader> loader_;
  std::unique_ptr<ModuleImporter> importer_;

  Tagged<PyDict> modules_{kNullAddress};
  Tagged<PyList> path_{kNullAddress};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_MANAGER_H_
