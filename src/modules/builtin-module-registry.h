// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_
#define SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_

#include <string_view>
#include <vector>

#include "src/modules/builtin-module.h"

namespace saauso::internal {

// BuiltinModuleRegistry 负责管理内建模块（builtin module）的注册表。
// 它只保存 name -> init_func 的映射，不执行模块初始化、不触碰 sys.modules。
class BuiltinModuleRegistry final {
 public:
  BuiltinModuleRegistry() = default;
  BuiltinModuleRegistry(const BuiltinModuleRegistry&) = delete;
  BuiltinModuleRegistry& operator=(const BuiltinModuleRegistry&) = delete;
  ~BuiltinModuleRegistry() = default;

  // 注册一个内建模块初始化函数。
  //
  // 注意：BuiltinModuleRegistry 会以 std::string_view 的形式保存 name，
  // 因此 name 必须具有足够长的生命周期（推荐使用字符串字面量）。
  void Register(std::string_view name, BuiltinModuleInitFunc init);
  BuiltinModuleInitFunc Find(std::string_view name) const;

 private:
  struct Entry {
    std::string_view name;
    BuiltinModuleInitFunc init{nullptr};
  };
  std::vector<Entry> entries_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_REGISTRY_H_
