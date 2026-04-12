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

// 将 import 语句里的 (name, level, globals) 组合解析成最终模块全名。
// 这个模块只负责“名字解析”，不负责在文件系统里查找模块，也不负责执行模块。
//
// 典型例子：
// - import pkg.sub
//   => name="pkg.sub", level=0，结果仍然是 "pkg.sub"
// - from . import util
//   若 globals 表示当前模块属于包 "pkg.tools"
//   => name="util", level=1，结果是 "pkg.tools.util"
// - from ..core import api
//   若 globals 表示当前模块属于包 "pkg.tools.impl"
//   => name="core", level=2，结果是 "pkg.core"
//   随后 IMPORT_FROM 再从 "pkg.core" 中取出属性 "api"
// - from .. import helper
//   若 globals 表示当前模块属于包 "pkg.tools"
//   => name="" 或 null, level=2，结果是 "pkg"
//
// 它同时负责校验相对导入是否合法：
// - 没有已知父包时，报 "attempted relative import with no known parent
//   package"
// - 越过顶层包继续向上跳时，报 "attempted relative import beyond
//   top-level package"
class ModuleNameResolver {
 public:
  ModuleNameResolver(Isolate* isolate) : isolate_(isolate) {}

  ModuleNameResolver(ModuleNameResolver&) = delete;
  ModuleNameResolver& operator=(ModuleNameResolver&) = delete;

  // 对外主入口：返回 import 语句真正要加载的模块名。
  // - 绝对导入：直接返回 name
  //   例如 import a.b => "a.b"
  // - 相对导入：结合 globals 中的 __package__/__name__/__path__ 计算
  //   例如当前模块是 pkg.tools.runner，from ..core import util
  //   会先得到 "pkg.core"
  // - name 允许为空字符串：仅当 level > 0（相对导入）时成立
  //   例如 from .. import helper，会先把目标模块解析为 "pkg"
  // 失败时设置 pending exception 并返回空 MaybeHandle。
  MaybeHandle<PyString> ResolveFullName(Handle<PyString> name,
                                        int64_t level,
                                        Handle<PyDict> globals);

 private:
  // 从 "pkg.tools.impl" 提取父模块名 "pkg.tools"；
  // 如果已经没有父模块，则返回空串。
  Handle<PyString> ParentModuleNameOrEmpty(Handle<PyString> name);

  // 从调用方 globals 推断“当前模块所在包”。
  // - 优先使用 __package__
  // - 否则退回到 __name__，并结合是否存在 __path__ 判断当前模块是包还是
  //   普通模块
  // 例子：
  // - __package__="pkg.tools" => 返回 "pkg.tools"
  // - __name__="pkg.tools.mod" 且无 __path__ => 返回 "pkg.tools"
  // - __name__="pkg.tools" 且有 __path__ => 返回 "pkg.tools"
  MaybeHandle<PyString> ResolvePackageFromGlobals(Handle<PyDict> globals);

  // 处理 level > 0 的相对导入。
  // 例子：
  // - 当前包 "pkg.tools.impl"，name="util"，level=1
  //   => "pkg.tools.impl.util"
  // - 当前包 "pkg.tools.impl"，name="core"，level=2
  //   => "pkg.tools.core"
  // - 当前包 "pkg.tools.impl"，name=""，level=3
  //   => "pkg"
  MaybeHandle<PyString> ResolveRelativeImportName(Handle<PyString> name,
                                                  int64_t level,
                                                  Handle<PyDict> globals);

  Isolate* isolate_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_NAME_RESOLVER_H_
