// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_IMPORTER_H_
#define SAAUSO_MODULES_MODULE_IMPORTER_H_

#include <cstdint>

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class ModuleManager;
class PyDict;
class PyList;
class PyObject;
class PyString;
class PyModule;
class PyTuple;

class ModuleImporter final {
 public:
  ModuleImporter(ModuleManager* manager);
  ModuleImporter(const ModuleImporter&) = delete;
  ModuleImporter& operator=(const ModuleImporter&) = delete;
  ~ModuleImporter() = default;

  // 导入模块，并按 CPython 语义应用 fromlist 的返回值规则。
  // 失败时设置 pending exception 并返回空 MaybeHandle。
  // - name: IMPORT_NAME 的 name 参数（可能是相对导入名）。
  // - fromlist: IMPORT_NAME 的 fromlist 参数，影响返回顶层包还是最后导入模块。
  // - level: 相对导入层级；0 表示绝对导入。
  // - globals: 当前执行上下文的 globals，用于解析相对导入基准包信息。
  MaybeHandle<PyModule> ImportModule(Handle<PyString> name,
                                      Handle<PyTuple> fromlist,
                                      int64_t level,
                                      Handle<PyDict> globals);

 private:
  // 导入一个完整的绝对模块名（可能为 dotted-name），并返回最后一段模块对象。
  MaybeHandle<PyModule> ImportModuleImpl(Handle<PyString> fullname);

  // 从 sys.modules 中获取模块；若未命中则按 search path 规则触发加载。
  // - part_fullname: 当前导入段的累积全名（例如 a.b.c 的第二段是 a.b）。
  // - is_top: 是否为顶层段（顶层使用 sys.path；非顶层使用
  // parent_module.__path__）。
  // - parent_module: 非顶层段对应的父模块（顶层段允许为 null）。
  MaybeHandle<PyModule> GetOrLoadModulePart(Handle<PyString> part_fullname,
                                            bool is_top,
                                            Handle<PyObject> parent_module);

  // 选择当前段导入应使用的搜索路径列表。
  // 顶层段返回 sys.path；非顶层段返回 parent_module.__path__。
  // 失败时（如 parent 非 package）设置 pending exception 并返回空 MaybeHandle。
  MaybeHandle<PyList> SelectSearchPathList(bool is_top,
                                           Handle<PyObject> parent_module);

  // 将 child_module 绑定到 parent_module 的命名空间中：parent.child_short_name
  // = child_module。 该步骤用于对齐 dotted-name 导入语义，并提升 IMPORT_FROM 的
  // fast path 命中率。
  void BindChildModuleToParentNamespace(Handle<PyObject> parent_module,
                                        Handle<PyString> child_short_name,
                                        Handle<PyObject> child_module);

  // 如果当前段不是最后一段，则校验 module 必须为 package（存在 __path__
  // list）。若校验失败则设置 ImportError 并返回 false；成功返回 true。
  bool EnsurePackageForNextSegment(Handle<PyObject> module,
                                   Handle<PyString> module_fullname);

  // 应用 IMPORT_NAME 的返回值语义：
  // - fromlist 为空且 fullname 含 dot 时，返回顶层包；
  // - 否则返回最后导入的模块对象。
  Handle<PyModule> ApplyImportReturnSemantics(Handle<PyString> fullname,
                                              Handle<PyTuple> fromlist,
                                              Handle<PyModule> last_module);

  Handle<PyDict> modules_dict();

  ModuleManager* manager_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_IMPORTER_H_
