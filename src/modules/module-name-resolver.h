// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_
#define SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyDict;
class PyString;

class ModuleNameResolver {
 public:
  ModuleNameResolver(Isolate* isolate) : isolate_(isolate) {}

  ModuleNameResolver(ModuleNameResolver&) = delete;
  ModuleNameResolver& operator=(ModuleNameResolver&) = delete;

  // 解析并返回最终导入的模块全名。
  // - name 允许为空字符串：仅当 level > 0（相对导入）时成立。
  // 失败时设置 pending exception 并返回空 MaybeHandle。
  MaybeHandle<PyString> ResolveFullName(Handle<PyString> name,
                                        int64_t level,
                                        Handle<PyDict> globals);

 private:
  Handle<PyString> ParentModuleNameOrEmpty(Handle<PyString> name);
  MaybeHandle<PyString> ResolvePackageFromGlobals(Handle<PyDict> globals);
  MaybeHandle<PyString> ResolveRelativeImportName(Handle<PyString> name,
                                                  int64_t level,
                                                  Handle<PyDict> globals);

  Isolate* isolate_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_
