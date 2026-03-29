// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_
#define SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_

#include <string_view>
#include <vector>

#include "src/handles/handles.h"
#include "src/modules/builtin-module.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Isolate;

class PyString;
class ObjectVisitor;

// BuiltinModuleRegistry 负责管理内建模块（builtin module）的注册表。
//
// 设计要点：
// - 它保存 name -> init_func 的映射，不执行模块初始化、不触碰 sys.modules；
// - 它同时提供“默认 builtin 集合”的装配入口（bootstrap），用于把 VM 内置的
//   builtin 模块清单注册进该 registry，避免 ModuleManager 直接依赖具体清单。
class BuiltinModuleRegistry final {
 public:
  BuiltinModuleRegistry(Isolate* isolate) : isolate_(isolate) {};
  BuiltinModuleRegistry(const BuiltinModuleRegistry&) = delete;
  BuiltinModuleRegistry& operator=(const BuiltinModuleRegistry&) = delete;
  ~BuiltinModuleRegistry() = default;

  // 注册一个内建模块初始化函数。
  void Register(Handle<PyString> name, BuiltinModuleInitFunc init);
  BuiltinModuleInitFunc Find(Handle<PyString> name) const;

  // 注册 SAAUSO 默认提供的所有 builtin 模块。
  //
  // 说明：
  // - 该方法只负责向 registry 注册 init 函数指针；
  // - 模块对象的创建与 sys.modules 写入仍发生在 ModuleLoader 的 import 路径中。
  void BootstrapAllBuiltinModules();

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  struct Entry {
    Tagged<PyString> name;
    BuiltinModuleInitFunc init{nullptr};
  };
  Vector<Entry> entries_;
  Isolate* isolate_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_
