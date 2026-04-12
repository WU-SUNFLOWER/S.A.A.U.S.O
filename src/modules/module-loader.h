// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_LOADER_H_
#define SAAUSO_MODULES_MODULE_LOADER_H_

#include <cstddef>
#include <string>

#include "src/handles/maybe-handles.h"
#include "src/modules/builtin-module.h"

namespace saauso::internal {

class Isolate;
class BuiltinModuleRegistry;
class ModuleFinder;
class ModuleManager;
class PyList;
class PyDict;
class PyObject;
class PyString;
struct ModuleLocation;

// 根据“已经解析好的模块全名”真正把模块对象加载出来。
//
// 这个模块负责 import 流程里的“装载与执行”阶段：
// - 先判断是不是 builtin 模块
// - builtin 未命中时，再去文件系统查找 .py / .pyc / package
// - 找到后创建 PyModule，初始化 __name__/__package__/__file__/__path__
// - 执行模块代码，并把结果放入 ModuleManager 的 modules 缓存
//
// 典型例子：
// - import sys
//   => 命中 builtin registry，直接构造并返回内建 sys 模块
// - import pkg.util
//   => 在 search_path_list 下查找 util.py / util.pyc 或 package 目录，
//      执行后返回模块对象
// - import pkg
//   => 若命中 pkg/__init__.py，则把它当作 package 加载，并写入 __path__
//
// 它假设 fullname 已经是最终模块名。
// 例如相对导入 from ..core import api 中，传进来的 fullname 应该已经是
// "pkg.core"，而不是原始的相对名字 "..core"。
class ModuleLoader final {
 public:
  ModuleLoader(Isolate* isolate,
               ModuleFinder* finder,
               ModuleManager* manager,
               BuiltinModuleRegistry* builtin_registry);
  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;
  ~ModuleLoader() = default;

  // 加载 fullname 对应的“一个模块部分”，并返回 PyModule。
  //
  // 例子：
  // - fullname="sys"       => 返回 builtin sys 模块
  // - fullname="pkg.util"  => 从文件系统加载 pkg.util
  // - fullname="pkg"       => 若命中 pkg/__init__.py，则返回包模块 pkg
  //
  // 若未找到模块，则抛出 ModuleNotFoundError；
  // 若查找、读文件或执行阶段失败，则设置 pending exception 并返回空
  // MaybeHandle。
  MaybeHandle<PyModule> LoadModulePart(Handle<PyString> fullname,
                                       Handle<PyList> search_path_list);

 private:
  // 内部主流程：
  // - 成功时返回模块对象
  // - 未找到时返回 None
  // - 发生异常时返回空 MaybeHandle
  MaybeHandle<PyObject> LoadModulePartOrNoneImpl(
      Handle<PyString> fullname,
      Handle<PyList> search_path_list);

  // 已经拿到 ModuleLocation 后，按文件类型分发执行：
  // - .py  => 读源码并解释执行
  // - .pyc => 直接执行字节码
  MaybeHandle<PyObject> ExecuteModuleOrNoneInternal(Handle<PyString> fullname,
                                                    const ModuleLocation& loc);

  // 尝试按 builtin 模块加载；未命中时返回 None。
  // 例如 fullname="sys"、"math" 时可能在这里直接成功。
  MaybeHandle<PyObject> LoadAsBuiltinModuleOrNone(Handle<PyString> fullname);

  // 尝试按文件模块加载；未命中时返回 None。
  // 它会先把 fullname 的最后一段取出来作为相对名去查找，例如：
  // - fullname="pkg.util" => relative_name="util"
  // - fullname="pkg"      => relative_name="pkg"
  MaybeHandle<PyObject> LoadAsFileModuleOrNone(Handle<PyString> fullname,
                                               Handle<PyList> search_path_list);

  // 用源码执行模块。
  // 例子：命中 ".../pkg/util.py" 后，创建模块字典并执行该 .py 源码。
  MaybeHandle<PyModule> ExecuteModuleFromSource(Handle<PyString> fullname,
                                                const ModuleLocation& loc,
                                                Handle<PyString> source);

  // 用 .pyc 执行模块。
  // 例子：命中 ".../pkg/util.pyc" 后，创建模块字典并执行该字节码文件。
  MaybeHandle<PyModule> ExecuteModuleFromPyc(Handle<PyString> fullname,
                                             const ModuleLocation& loc);

  // 初始化模块字典里的导入相关元信息。
  // 典型字段包括：
  // - __name__    : 模块全名，例如 "pkg.util"
  // - __package__ : 所属包名，例如 "pkg"
  // - __file__    : 模块来源文件
  // - __path__    : 仅 package 才有，例如 [".../pkg"]
  MaybeHandle<PyObject> InitializeModuleDict(Handle<PyModule> module,
                                             Handle<PyString> fullname,
                                             const ModuleLocation& loc);

  Isolate* isolate_{nullptr};
  ModuleFinder* finder_{nullptr};
  ModuleManager* manager_{nullptr};
  BuiltinModuleRegistry* builtin_registry_{nullptr};
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_LOADER_H_
