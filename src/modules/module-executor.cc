// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-executor.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

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
  Handle<PyObject> module_obj(module);
  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);

  PyDict::Put(module_dict, PyString::NewInstance("__name__"), fullname);

  if (loc.is_package) {
    PyDict::Put(module_dict, PyString::NewInstance("__package__"), fullname);
  } else {
    std::string_view fullname_view = ModuleUtils::ToStringView(fullname);
    size_t dot = fullname_view.rfind('.');
    if (dot == std::string_view::npos) {
      PyDict::Put(module_dict, PyString::NewInstance("__package__"),
                  PyString::NewInstance(""));
    } else {
      PyDict::Put(module_dict, PyString::NewInstance("__package__"),
                  PyString::NewInstance(fullname_view.data(),
                                        static_cast<int64_t>(dot)));
    }
  }

  PyDict::Put(module_dict, PyString::NewInstance("__file__"),
              PyString::NewInstance(loc.origin.c_str(),
                                    static_cast<int64_t>(loc.origin.size())));

  PyDict::Put(module_dict, ST(class),
              PyObject::GetKlass(module_obj)->type_object());

  if (loc.is_package) {
    Handle<PyList> pkg_path = PyList::NewInstance();
    PyList::Append(pkg_path, PyString::NewInstance(
                                 loc.package_dir.c_str(),
                                 static_cast<int64_t>(loc.package_dir.size())));
    PyDict::Put(module_dict, PyString::NewInstance("__path__"), pkg_path);
  }
}

}  // namespace

ModuleExecutor::ModuleExecutor(Isolate* isolate,
                               ModuleFinder* finder,
                               ModuleManager* manager,
                               BuiltinModuleRegistry* builtin_registry)
    : isolate_(isolate),
      finder_(finder),
      manager_(manager),
      builtin_registry_(builtin_registry) {}

Handle<PyObject> ModuleExecutor::LoadModulePart(
    Handle<PyString> fullname,
    const std::vector<std::string>& search_paths) {
  EscapableHandleScope scope;

  BuiltinModuleInitFunc builtin_init = nullptr;
  if (builtin_registry_ != nullptr) {
    builtin_init = builtin_registry_->Find(ModuleUtils::ToStringView(fullname));
  }
  if (builtin_init != nullptr) {
    Handle<PyModule> module = builtin_init(isolate_, manager_);
    Handle<PyObject> module_obj(module);
    Handle<PyDict> modules_dict = manager_->modules();
    PyDict::Put(modules_dict, fullname, module_obj);
    return scope.Escape(module_obj);
  }

  std::string_view fullname_view = ModuleUtils::ToStringView(fullname);
  size_t dot = fullname_view.rfind('.');
  std::string_view relative_name = dot == std::string_view::npos
                                       ? fullname_view
                                       : fullname_view.substr(dot + 1);

  ModuleLocation loc = finder_->FindModuleLocation(search_paths, relative_name);
  if (loc.origin.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: No module named '%.*s'\n",
                 static_cast<int>(fullname_view.size()), fullname_view.data());
    std::exit(1);
  }

  std::string source;
  if (!finder_->ReadModuleSource(loc, &source)) {
    std::fprintf(stderr, "ImportError: cannot read '%s'\n", loc.origin.c_str());
    std::exit(1);
  }

  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, loc);

  Handle<PyDict> modules_dict = manager_->modules();
  PyDict::Put(modules_dict, fullname, module);

  Handle<PyDict> module_dict = PyObject::GetProperties(module);
  Handle<PyFunction> boilerplate = Compiler::CompileSource(
      isolate_, std::string_view(source), std::string_view(loc.origin));
  boilerplate->set_func_globals(module_dict);
  isolate_->interpreter()->CallPython(boilerplate, Handle<PyObject>::null(),
                                      Handle<PyTuple>::null(),
                                      Handle<PyDict>::null(), module_dict);

  return scope.Escape(module);
}

}  // namespace saauso::internal
