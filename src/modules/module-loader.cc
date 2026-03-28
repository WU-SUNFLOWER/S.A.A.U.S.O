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
  EscapableHandleScope scope;

  Handle<PyObject> loaded;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, loaded, LoadModulePartOrNoneImpl(fullname, search_path_list));

  if (IsPyNone(loaded)) {
    Runtime_ThrowErrorf(isolate_, ExceptionType::kModuleNotFoundError,
                        "No module named '%s'", fullname->buffer());
    return kNullMaybe;
  }

  auto loaded_module = Handle<PyModule>::cast(loaded);

  // 将加载的模块保存到 modules 缓存
  Handle<PyDict> modules_dict = manager_->modules();
  RETURN_ON_EXCEPTION(
      isolate_, PyDict::Put(modules_dict, fullname, loaded_module, isolate_));

  return scope.Escape(loaded_module);
}

MaybeHandle<PyObject> ModuleLoader::LoadModulePartOrNoneImpl(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  // 先尝试查找是否命中 builtin 模块
  Handle<PyObject> builtin_module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, builtin_module,
                             LoadAsBuiltinModuleOrNone(fullname));
  if (!IsPyNone(builtin_module)) {
    return builtin_module;
  }

  // 不是 builtin 模块，走 OS 文件系统进行查找
  return LoadAsFileModuleOrNone(fullname, search_path_list);
}

MaybeHandle<PyObject> ModuleLoader::LoadAsBuiltinModuleOrNone(
    Handle<PyString> fullname) {
  BuiltinModuleInitFunc builtin_init = builtin_registry_->Find(fullname);
  if (builtin_init == nullptr) {
    return handle(isolate_->py_none_object());
  }

  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             builtin_init(isolate_, manager_));

  return module;
}

MaybeHandle<PyObject> ModuleLoader::LoadAsFileModuleOrNone(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  auto dot_index = fullname->LastIndexOf(ST(dot));
  Handle<PyString> relative_name;
  if (dot_index == PyString::kNotFound) {
    relative_name = fullname;
  } else {
    relative_name = PyString::Slice(fullname, dot_index + 1);
  }

  ModuleLocation loc;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, loc,
      finder_->FindModuleLocation(search_path_list, relative_name));

  if (loc.origin.empty()) [[unlikely]] {
    return handle(isolate_->py_none_object());
  }

  return ExecuteModuleOrNoneInternal(fullname, loc);
}

MaybeHandle<PyObject> ModuleLoader::ExecuteModuleOrNoneInternal(
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  if (loc.kind == ModuleFileKind::kSourcePy) {
    Handle<PyString> source;
    if (!finder_->ReadModuleSource(loc, source)) {
      Runtime_ThrowErrorf(isolate_, ExceptionType::kImportError,
                          "cannot read '%s'", loc.origin.c_str());
      return kNullMaybe;
    }
    return ExecuteModuleFromSource(fullname, loc, source);
  }

  if (loc.kind == ModuleFileKind::kBytecodePyc) {
    return ExecuteModuleFromPyc(fullname, loc);
  }

  return handle(isolate_->py_none_object());
}

MaybeHandle<PyModule> ModuleLoader::ExecuteModuleFromSource(
    Handle<PyString> fullname,
    const ModuleLocation& loc,
    Handle<PyString> source) {
  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             isolate_->factory()->NewPyModule());

  RETURN_ON_EXCEPTION(isolate_, InitializeModuleDict(module, fullname, loc));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  RETURN_ON_EXCEPTION(
      isolate_, Runtime_ExecutePythonSourceCode(isolate_, source, module_dict,
                                                module_dict, loc.origin));

  return module;
}

MaybeHandle<PyModule> ModuleLoader::ExecuteModuleFromPyc(
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  Handle<PyModule> module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, module,
                             isolate_->factory()->NewPyModule());

  RETURN_ON_EXCEPTION(isolate_, InitializeModuleDict(module, fullname, loc));

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  RETURN_ON_EXCEPTION(
      isolate_, Runtime_ExecutePythonPycFile(isolate_, loc.origin, module_dict,
                                             module_dict));

  return module;
}

MaybeHandle<PyObject> ModuleLoader::InitializeModuleDict(
    Handle<PyModule> module,
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  RETURN_ON_EXCEPTION(isolate_,
                      PyDict::Put(module_dict, ST(name), fullname, isolate_));

  if (loc.is_package) {
    RETURN_ON_EXCEPTION(
        isolate_, PyDict::Put(module_dict, ST(package), fullname, isolate_));
  } else {
    int64_t dot_index = fullname->LastIndexOf(ST(dot));
    if (dot_index == PyString::kNotFound) {
      RETURN_ON_EXCEPTION(isolate_,
                          PyDict::Put(module_dict, ST(package),
                                      PyString::NewInstance(""), isolate_));
    } else {
      int64_t package_end = static_cast<int64_t>(dot_index) - 1;
      Handle<PyString> package_name = PyString::Slice(fullname, 0, package_end);

      RETURN_ON_EXCEPTION(isolate_, PyDict::Put(module_dict, ST(package),
                                                package_name, isolate_));
    }
  }

  RETURN_ON_EXCEPTION(
      isolate_, PyDict::Put(module_dict, ST(file),
                            ModuleUtils::NewPyString(loc.origin), isolate_));

  if (loc.is_package) {
    Handle<PyList> pkg_path = PyList::New(isolate_);
    PyList::Append(pkg_path, ModuleUtils::NewPyString(loc.package_dir),
                   isolate_);
    RETURN_ON_EXCEPTION(isolate_,
                        PyDict::Put(module_dict, ST(path), pkg_path, isolate_));
  }

  return handle(isolate_->py_none_object());
}

}  // namespace saauso::internal
