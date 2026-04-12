// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-loader.h"

#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/modules/builtin-module-registry.h"
#include "src/modules/module-finder.h"
#include "src/modules/module-manager.h"
#include "src/modules/module-utils.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-exec.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

ModuleLoader::ModuleLoader(Isolate* isolate,
                           ModuleFinder* finder,
                           ModuleManager* manager,
                           BuiltinModuleRegistry* builtin_registry)
    : isolate_(isolate),
      finder_(finder),
      manager_(manager),
      builtin_registry_(builtin_registry) {}

MaybeHandle<PyModule> ModuleLoader::LoadModulePart(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  EscapableHandleScope scope(isolate_);

  // 先尝试把 fullname 真正加载成模块对象：
  // - 找到并成功执行 => 返回模块
  // - 未找到         => 返回 None
  // - 出现异常       => 返回空 MaybeHandle
  Handle<PyObject> loaded;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, loaded, LoadModulePartOrNoneImpl(fullname, search_path_list));

  // 对外接口不把“未找到”保留为 None，而是统一提升成
  // ModuleNotFoundError。
  // 例如 import missing_pkg 时，会在这里抛出：
  //   No module named 'missing_pkg'
  if (IsPyNone(loaded, isolate_)) {
    Runtime_ThrowErrorf(isolate_, ExceptionType::kModuleNotFoundError,
                        "No module named '%s'", fullname->buffer());
    return kNullMaybe;
  }

  auto loaded_module = Handle<PyModule>::cast(loaded);

  // 将加载成功的模块写入 modules 缓存（相当于 sys.modules 主表）。
  // 例如 import pkg.util 成功后，会记录：
  //   modules["pkg.util"] = <module pkg.util>
  Handle<PyDict> modules_dict = manager_->modules();
  RETURN_ON_EXCEPTION(
      isolate_, PyDict::Put(modules_dict, fullname, loaded_module, isolate_));

  return scope.Escape(loaded_module);
}

MaybeHandle<PyObject> ModuleLoader::LoadModulePartOrNoneImpl(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  // 加载顺序与 CPython 的常见思路一致：
  // 1. 先看内建模块注册表
  // 2. 未命中时再去文件系统
  //
  // 例子：
  // - fullname="sys"       => 一般在步骤 1 命中
  // - fullname="pkg.util"  => 一般会落到步骤 2
  Handle<PyObject> builtin_module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, builtin_module,
                             LoadAsBuiltinModuleOrNone(fullname));
  if (!IsPyNone(builtin_module, isolate_)) {
    return builtin_module;
  }

  // 不是 builtin 模块，走 OS 文件系统进行查找
  return LoadAsFileModuleOrNone(fullname, search_path_list);
}

MaybeHandle<PyObject> ModuleLoader::LoadAsBuiltinModuleOrNone(
    Handle<PyString> fullname) {
  // builtin_registry_ 维护 VM 预注册的内建模块工厂。
  // 例如 "sys"、"math"、"time" 等可能在这里被直接构造出来，
  // 不需要访问磁盘。
  BuiltinModuleInitFunc builtin_init = builtin_registry_->Find(fullname);
  if (builtin_init == nullptr) {
    return isolate_->factory()->py_none_object();
  }

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             builtin_init(isolate_, manager_));

  return module;
}

MaybeHandle<PyObject> ModuleLoader::LoadAsFileModuleOrNone(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  // 文件系统查找只需要 fullname 的最后一段作为“相对文件名”。
  // 例如：
  // - fullname="pkg.util"       => relative_name="util"
  // - fullname="pkg.sub.helper" => relative_name="helper"
  //
  // 这是因为父包路径已经由 import 主流程逐层推进；
  // 当前这一层只需要在给定 search_path_list 下找本段模块。
  auto dot_index = fullname->LastIndexOf(ST(dot, isolate_));
  Handle<PyString> relative_name;
  if (dot_index == PyString::kNotFound) {
    relative_name = fullname;
  } else {
    relative_name = PyString::Slice(fullname, dot_index + 1, isolate_);
  }

  ModuleLocation loc;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, loc,
      finder_->FindModuleLocation(search_path_list, relative_name));

  // 没找到时用 None 表示“本层未命中”，交给上层决定是否抛错。
  // 例如在 search_path_list 里既没有 util.py，也没有 util/__init__.py，
  // 就会走这里。
  if (loc.origin.empty()) [[unlikely]] {
    return isolate_->factory()->py_none_object();
  }

  // 找到后再根据实际文件类型执行。
  return ExecuteModuleOrNoneInternal(fullname, loc);
}

MaybeHandle<PyObject> ModuleLoader::ExecuteModuleOrNoneInternal(
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  // 源码模块：先读源码，再走源码执行入口。
  // 例子：命中 ".../pkg/util.py"
  if (loc.kind == ModuleFileKind::kSourcePy) {
    Handle<PyString> source;
    if (!finder_->ReadModuleSource(loc, source)) {
      Runtime_ThrowErrorf(isolate_, ExceptionType::kImportError,
                          "cannot read '%s'", loc.origin.c_str());
      return kNullMaybe;
    }
    return ExecuteModuleFromSource(fullname, loc, source);
  }

  // 字节码模块：直接执行 .pyc。
  // 例子：命中 ".../pkg/util.pyc"
  if (loc.kind == ModuleFileKind::kBytecodePyc) {
    return ExecuteModuleFromPyc(fullname, loc);
  }

  return isolate_->factory()->py_none_object();
}

MaybeHandle<PyModule> ModuleLoader::ExecuteModuleFromSource(
    Handle<PyString> fullname,
    const ModuleLocation& loc,
    Handle<PyString> source) {
  // 每次导入成功都要先创建一个新的模块对象，并填充基础元信息。
  // 例如 fullname="pkg.util" 时，后续至少会写入：
  // - __name__    = "pkg.util"
  // - __package__ = "pkg"
  // - __file__    = ".../pkg/util.py"
  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             isolate_->factory()->NewPyModule());

  RETURN_ON_EXCEPTION(isolate_, InitializeModuleDict(module, fullname, loc));

  Handle<PyDict> module_dict = PyObject::GetProperties(module, isolate_);
  RETURN_ON_EXCEPTION(
      isolate_, Runtime_ExecutePythonSourceCode(isolate_, source, module_dict,
                                                module_dict, loc.origin));

  // 执行完成后，模块级顶层定义都已经落入 module_dict。
  return module;
}

MaybeHandle<PyModule> ModuleLoader::ExecuteModuleFromPyc(
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  // .pyc 路径与源码路径的核心差别，只在于执行入口不同；
  // 模块对象创建与 __dict__ 初始化逻辑保持一致。
  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             isolate_->factory()->NewPyModule());

  RETURN_ON_EXCEPTION(isolate_, InitializeModuleDict(module, fullname, loc));

  Handle<PyDict> module_dict = PyObject::GetProperties(module, isolate_);
  RETURN_ON_EXCEPTION(
      isolate_, Runtime_ExecutePythonPycFile(isolate_, loc.origin, module_dict,
                                             module_dict));

  return module;
}

MaybeHandle<PyObject> ModuleLoader::InitializeModuleDict(
    Handle<PyModule> module,
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  Handle<PyDict> module_dict = PyObject::GetProperties(module, isolate_);

  // __name__ 总是模块全名。
  // 例如：
  // - 顶层模块 util     => "util"
  // - 子模块 pkg.util   => "pkg.util"
  RETURN_ON_EXCEPTION(isolate_, PyDict::Put(module_dict, ST(name, isolate_),
                                            fullname, isolate_));

  if (loc.is_package) {
    // package 的 __package__ 等于它自己的全名。
    // 例如加载 pkg/__init__.py 时：
    // - __name__    = "pkg"
    // - __package__ = "pkg"
    RETURN_ON_EXCEPTION(
        isolate_,
        PyDict::Put(module_dict, ST(package, isolate_), fullname, isolate_));
  } else {
    // 普通模块的 __package__ 是它的父包名。
    // 例如：
    // - fullname="pkg.util" => __package__="pkg"
    // - fullname="util"     => __package__=""
    int64_t dot_index = fullname->LastIndexOf(ST(dot, isolate_));
    if (dot_index == PyString::kNotFound) {
      RETURN_ON_EXCEPTION(isolate_,
                          PyDict::Put(module_dict, ST(package, isolate_),
                                      PyString::New(isolate_, ""), isolate_));
    } else {
      int64_t package_end = static_cast<int64_t>(dot_index) - 1;
      Handle<PyString> package_name =
          PyString::Slice(fullname, 0, package_end, isolate_);

      RETURN_ON_EXCEPTION(isolate_,
                          PyDict::Put(module_dict, ST(package, isolate_),
                                      package_name, isolate_));
    }
  }

  RETURN_ON_EXCEPTION(
      isolate_,
      PyDict::Put(module_dict, ST(file, isolate_),
                  PyString::FromStdString(isolate_, loc.origin), isolate_));

  if (loc.is_package) {
    // 只有包才需要 __path__，供后续继续查找子模块。
    // 例如加载 pkg/__init__.py 后：
    // - __path__ = [".../pkg"]
    Handle<PyList> pkg_path = PyList::New(isolate_);
    PyList::Append(pkg_path, PyString::FromStdString(isolate_, loc.package_dir),
                   isolate_);
    RETURN_ON_EXCEPTION(isolate_, PyDict::Put(module_dict, ST(path, isolate_),
                                              pkg_path, isolate_));
  }

  return isolate_->factory()->py_none_object();
}

}  // namespace saauso::internal
