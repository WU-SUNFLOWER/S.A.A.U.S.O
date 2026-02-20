// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-loader.h"

#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
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
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exec.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

void InitializeModuleDict(Handle<PyModule> module,
                          Handle<PyString> fullname,
                          const ModuleLocation& loc) {
  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  PyDict::Put(module_dict, ST(name), fullname);

  if (loc.is_package) {
    PyDict::Put(module_dict, ST(package), fullname);
  } else {
    int64_t dot_index = fullname->LastIndexOf(ST(dot));
    if (dot_index == PyString::kNotFound) {
      PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));
    } else {
      int64_t package_end = static_cast<int64_t>(dot_index) - 1;
      Handle<PyString> package_name = PyString::Slice(fullname, 0, package_end);
      PyDict::Put(module_dict, ST(package), package_name);
    }
  }

  PyDict::Put(module_dict, PyString::NewInstance("__file__"),
              ModuleUtils::NewPyString(loc.origin));

  PyDict::Put(module_dict, ST(class),
              PyObject::GetKlass(module)->type_object());

  if (loc.is_package) {
    Handle<PyList> pkg_path = PyList::NewInstance();
    PyList::Append(pkg_path, ModuleUtils::NewPyString(loc.package_dir));
    PyDict::Put(module_dict, ST(path), pkg_path);
  }
}

}  // namespace

ModuleLoader::ModuleLoader(Isolate* isolate,
                           ModuleFinder* finder,
                           ModuleManager* manager,
                           BuiltinModuleRegistry* builtin_registry)
    : isolate_(isolate),
      finder_(finder),
      manager_(manager),
      builtin_registry_(builtin_registry) {}

Handle<PyModule> ModuleLoader::LoadModulePart(Handle<PyString> fullname,
                                              Handle<PyList> search_path_list) {
  EscapableHandleScope scope;

  Handle<PyModule> loaded_module =
      LoadModulePartImpl(fullname, search_path_list);

  if (loaded_module.is_null()) {
    std::fprintf(stderr, "ModuleNotFoundError: No module named '%s'\n",
                 fullname->buffer());
    std::exit(1);
    return Handle<PyModule>::null();
  }

  // 将加载的模块保存到modules缓存
  Handle<PyDict> modules_dict = manager_->modules();
  PyDict::Put(modules_dict, fullname, loaded_module);

  return scope.Escape(loaded_module);
}

Handle<PyModule> ModuleLoader::LoadModulePartImpl(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  // 先尝试查找是否命中builtin模块
  Handle<PyModule> module = LoadAsBuiltinModule(fullname);
  if (!module.is_null()) {
    return module;
  }

  // 不是builtin模块，走OS文件系统进行查找
  module = LoadAsFileModule(fullname, search_path_list);
  if (!module.is_null()) {
    return module;
  }

  return Handle<PyModule>::null();
}

Handle<PyModule> ModuleLoader::LoadAsBuiltinModule(Handle<PyString> fullname) {
  BuiltinModuleInitFunc builtin_init = builtin_registry_->Find(fullname);
  if (builtin_init == nullptr) {
    return Handle<PyModule>::null();
  }

  Handle<PyModule> module = builtin_init(isolate_, manager_);
  return module;
}

Handle<PyModule> ModuleLoader::LoadAsFileModule(
    Handle<PyString> fullname,
    Handle<PyList> search_path_list) {
  auto dot_index = fullname->LastIndexOf(ST(dot));
  Handle<PyString> relative_name;
  if (dot_index == PyString::kNotFound) {
    relative_name = fullname;
  } else {
    relative_name = PyString::Slice(fullname, dot_index + 1);
  }

  ModuleLocation loc =
      finder_->FindModuleLocation(search_path_list, relative_name);
  if (loc.origin.empty()) {
    return Handle<PyModule>::null();
  }

  return ExecuteModuleInternal(fullname, loc);
}

Handle<PyModule> ModuleLoader::ExecuteModuleInternal(
    Handle<PyString> fullname,
    const ModuleLocation& loc) {
  Handle<PyModule> module;
  if (loc.kind == ModuleFileKind::kSourcePy) {
    Handle<PyString> source;
    if (!finder_->ReadModuleSource(loc, source)) {
      std::fprintf(stderr, "ImportError: cannot read '%s'\n",
                   loc.origin.c_str());
      std::exit(1);
    }
    module = ExecuteModuleFromSource(fullname, loc, source);
    return module;
  }

  if (loc.kind == ModuleFileKind::kBytecodePyc) {
    module = ExecuteModuleFromPyc(fullname, loc);
    return module;
  }

  return Handle<PyModule>::null();
}

Handle<PyModule> ModuleLoader::ExecuteModuleFromSource(
    Handle<PyString> fullname,
    const ModuleLocation& loc,
    Handle<PyString> source) {
  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, loc);

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  Runtime_ExecutePythonSourceCode(source, module_dict, module_dict, loc.origin);

  return module;
}

Handle<PyModule> ModuleLoader::ExecuteModuleFromPyc(Handle<PyString> fullname,
                                                    const ModuleLocation& loc) {
  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, loc);

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  Runtime_ExecutePythonPycFile(loc.origin, module_dict, module_dict);

  return module;
}

}  // namespace saauso::internal
