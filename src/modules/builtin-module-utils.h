// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_
#define SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_

#include <cstdint>

#include "src/builtins/builtins-utils.h"
#include "src/handles/handles.h"
#include "src/objects/native-function-helpers.h"

namespace saauso::internal {

#define BUILTIN_MODULE_FUNC_NAME(name) BUILTIN_FUNC_NAME(name)
#define BUILTIN_MODULE_FUNC(name) BUILTIN(name)

struct BuiltinModuleFuncSpec {
  const char* name;
  NativeFuncPointer func;
};

// 抛出“内建模块函数不支持关键字参数”的统一 TypeError。
void ThrowNoKeywordArgsError(const char* module_name, const char* func_name);

// 在模块字典中安装单个内建模块函数。
Maybe<void> InstallBuiltinModuleFunc(Isolate* isolate,
                                     Handle<PyDict> module_dict,
                                     const char* name,
                                     NativeFuncPointer func);

// 按函数规格数组批量安装内建模块函数。
Maybe<void> InstallBuiltinModuleFuncsFromSpec(
    Isolate* isolate,
    Handle<PyDict> module_dict,
    const BuiltinModuleFuncSpec* specs,
    int64_t spec_count);

template <size_t N>
constexpr int64_t BuiltinModuleFuncSpecCount(
    const BuiltinModuleFuncSpec (&specs)[N]) {
  return static_cast<int64_t>(N);
}

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_

