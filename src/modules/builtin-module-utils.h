// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_
#define SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_

#include <cstdint>

#include "src/builtins/builtins-utils.h"
#include "src/common/globals.h"
#include "src/handles/handles.h"
#include "src/objects/native-function-helpers.h"

namespace saauso::internal {

#define BUILTIN_MODULE_FUNC_NAME(name) BUILTIN_FUNC_NAME(name)

// 特别提醒：
// 在 S.A.A.U.S.O MVP 版本的模块系统中，
// 内建模块中的函数被视为普通的Python函数（static），
// 而不是某个对象的方法（instance method）。
// 因此针对内建模块函数对应的C++实现函数，
// 其形参列表中的 receiver 为在 Python 代码执行正常的模块 API 调用时，
// 取值为 null，在 C++ 代码中请勿访问！
#define BUILTIN_MODULE_FUNC(name) BUILTIN(name)

#define BUILTIN_MODULE_ARGC(args) ((args).is_null() ? 0 : (args)->length())

#define BUILTIN_MODULE_EXPECT_NO_KWARGS_OR_RETURN(isolate, kwargs,          \
                                                  module_name, func_name)   \
  do {                                                                      \
    if (!(kwargs).is_null() && (kwargs)->occupied() != 0) {                 \
      BuiltinModuleUtils::ThrowNoKeywordArgsError((isolate), (module_name), \
                                                  (func_name));             \
      return kNullMaybe;                                                    \
    }                                                                       \
  } while (false)

#define BUILTIN_MODULE_EXPECT_ARGC_EQ_OR_RETURN(argc, expected, on_mismatch) \
  do {                                                                       \
    if ((argc) != (expected)) {                                              \
      on_mismatch;                                                           \
      return kNullMaybe;                                                     \
    }                                                                        \
  } while (false)

struct BuiltinModuleFuncSpec {
  const char* name;
  NativeFuncPointer func;
};

class BuiltinModuleUtils final : public AllStatic {
 public:
  // 创建内建模块对象，并初始化 __name__/__package__ 元信息。
  static MaybeHandle<PyModule> NewBuiltinModule(Isolate* isolate,
                                                const char* module_name,
                                                const char* package_name = "");

  // 抛出“内建模块函数不支持关键字参数”的统一 TypeError。
  static void ThrowNoKeywordArgsError(Isolate* isolate,
                                      const char* module_name,
                                      const char* func_name);

  // 在模块字典中安装单个内建模块函数。
  static Maybe<void> InstallBuiltinModuleFunc(Isolate* isolate,
                                              Handle<PyDict> module_dict,
                                              const char* name,
                                              NativeFuncPointer func);

  // 按函数规格数组批量安装内建模块函数。
  static Maybe<void> InstallBuiltinModuleFuncsFromSpec(
      Isolate* isolate,
      Handle<PyModule> module,
      const BuiltinModuleFuncSpec* specs,
      int64_t spec_count);

  template <size_t N>
  static constexpr int64_t BuiltinModuleFuncSpecCount(
      const BuiltinModuleFuncSpec (&specs)[N]) {
    return static_cast<int64_t>(N);
  }

 private:
  // 为内建模块字典写入 __name__/__package__ 元信息。
  static Maybe<void> InitializeBuiltinModule(Isolate* isolate,
                                             Handle<PyModule> module,
                                             const char* module_name,
                                             const char* package_name);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_BUILTIN_MODULE_UTILS_H_
