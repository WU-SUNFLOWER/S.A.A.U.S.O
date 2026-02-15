// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-loader.h"

#include <cstdio>
#include <cstdlib>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
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
    std::string_view fullname_view = ModuleUtils::ToStringView(fullname);
    size_t dot = fullname_view.rfind('.');
    if (dot == std::string_view::npos) {
      PyDict::Put(module_dict, ST(package), PyString::NewInstance(""));
    } else {
      int64_t package_end = static_cast<int64_t>(dot) - 1;
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

Handle<PyObject> ModuleLoader::LoadModulePart(Handle<PyString> fullname,
                                              Handle<PyList> search_path_list) {
  EscapableHandleScope scope;

  BuiltinModuleInitFunc builtin_init = nullptr;
  if (builtin_registry_ != nullptr) {
    builtin_init = builtin_registry_->Find(fullname);
  }
  if (builtin_init != nullptr) {
    Handle<PyModule> module = builtin_init(isolate_, manager_);
    Handle<PyObject> module_obj(module);
    Handle<PyDict> modules_dict = manager_->modules();
    PyDict::Put(modules_dict, fullname, module_obj);
    return scope.Escape(module_obj);
  }

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
    std::fprintf(stderr, "ModuleNotFoundError: No module named '%s'\n",
                 fullname->buffer());
    std::exit(1);
  }

  Handle<PyString> source;
  if (!finder_->ReadModuleSource(loc, source)) {
    std::fprintf(stderr, "ImportError: cannot read '%s'\n", loc.origin.c_str());
    std::exit(1);
  }

  Handle<PyModule> module = ExecuteModuleImpl(fullname, loc, source);
  return scope.Escape(module);
}

Handle<PyModule> ModuleLoader::ExecuteModuleImpl(Handle<PyString> fullname,
                                                 const ModuleLocation& loc,
                                                 Handle<PyString> source) {
  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, loc);

  Handle<PyDict> modules_dict = manager_->modules();
  PyDict::Put(modules_dict, fullname, module);

  Handle<PyDict> module_dict = PyObject::GetProperties(module);

  Handle<PyFunction> boilerplate =
      Compiler::CompileSource(isolate_, source, loc.origin);
  boilerplate->set_func_globals(module_dict);

  isolate_->interpreter()->CallPython(boilerplate, Handle<PyObject>::null(),
                                      Handle<PyTuple>::null(),
                                      Handle<PyDict>::null(), module_dict);

  return module;
}

}  // namespace saauso::internal
