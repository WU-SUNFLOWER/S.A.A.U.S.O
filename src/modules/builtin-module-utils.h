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
#define BUILTIN_MODULE_ARGC(args) ((args).is_null() ? 0 : (args)->length())

#define BUILTIN_MODULE_EXPECT_NO_KWARGS(kwargs, module_name, func_name) \
  do {                                                                  \
    if (!(kwargs).is_null() && (kwargs)->occupied() != 0) {             \
      ThrowNoKeywordArgsError((module_name), (func_name));              \
      return kNullMaybe;                                                \
    }                                                                   \
  } while (false)

#define BUILTIN_MODULE_EXPECT_ARGC_EQ(argc, expected, on_mismatch) \
  do {                                                             \
    if ((argc) != (expected)) {                                    \
      on_mismatch;                                                 \
      return kNullMaybe;                                           \
    }                                                              \
  } while (false)

struct BuiltinModuleFuncSpec {
  const char* name;
  NativeFuncPointer func;
};

// 为内建模块字典写入 __name__/__package__ 元信息。
Maybe<void> InitializeBuiltinModuleDict(Isolate* isolate,
                                        Handle<PyDict> module_dict,
                                        const char* module_name,
                                        const char* package_name = "");

// 创建内建模块对象，并初始化 __name__/__package__ 元信息。
MaybeHandle<PyModule> NewBuiltinModuleWithDefaultMeta(
    Isolate* isolate,
    const char* module_name,
    const char* package_name = "");

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
