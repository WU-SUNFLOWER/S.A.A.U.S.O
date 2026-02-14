// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_
#define SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_

#include <cstdint>

#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class PyDict;
class PyString;

class ModuleNameResolver : public AllStatic {
 public:
  // 解析并返回最终导入的模块全名。
  // - name 允许为空字符串：仅当 level > 0（相对导入）时成立。
  // - 返回值保证为 PyString（可能为空字符串，但此时调用方通常会报错并退出）。
  static Handle<PyString> ResolveFullName(Handle<PyString> name,
                                          int64_t level,
                                          Handle<PyDict> globals);

 private:
  static Handle<PyString> ParentModuleNameOrEmpty(Handle<PyString> name);
  static Handle<PyString> ResolvePackageFromGlobals(Handle<PyDict> globals);
  static Handle<PyString> ResolveRelativeImportName(Handle<PyString> name,
                                                    int64_t level,
                                                    Handle<PyDict> globals);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_
