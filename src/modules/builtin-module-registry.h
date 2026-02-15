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

class PyString;
class ObjectVisitor;

// BuiltinModuleRegistry 负责管理内建模块（builtin module）的注册表。
// 它只保存 name -> init_func 的映射，不执行模块初始化、不触碰 sys.modules。
class BuiltinModuleRegistry final {
 public:
  BuiltinModuleRegistry() = default;
  BuiltinModuleRegistry(const BuiltinModuleRegistry&) = delete;
  BuiltinModuleRegistry& operator=(const BuiltinModuleRegistry&) = delete;
  ~BuiltinModuleRegistry() = default;

  // 注册一个内建模块初始化函数。
  void Register(Handle<PyString> name, BuiltinModuleInitFunc init);
  BuiltinModuleInitFunc Find(Handle<PyString> name) const;

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  struct Entry {
    Tagged<PyString> name;
    BuiltinModuleInitFunc init{nullptr};
  };
  Vector<Entry> entries_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_
