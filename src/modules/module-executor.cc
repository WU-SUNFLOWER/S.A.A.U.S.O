// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-executor.h"

#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/modules/builtin-module-registry.h"
#include "src/modules/module-finder.h"
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

std::string ToStdString(Handle<PyString> s) {
  if (s.is_null()) {
    return std::string();
  }
  return std::string(s->buffer(), static_cast<size_t>(s->length()));
}

std::string JoinModuleName(const std::vector<std::string>& parts,
                           size_t count) {
  std::string out;
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      out.push_back('.');
    }
    out.append(parts[i]);
  }
  return out;
}

void InitializeModuleDict(Handle<PyModule> module,
                          Handle<PyString> fullname,
                          const std::vector<std::string>& parts,
                          size_t part_index,
                          const ModuleLocation& loc) {
  Handle<PyObject> module_obj(module);
  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);

  PyDict::Put(module_dict, PyString::NewInstance("__name__"), fullname);

  if (loc.is_package) {
    PyDict::Put(module_dict, PyString::NewInstance("__package__"), fullname);
  } else if (part_index == 0) {
    PyDict::Put(module_dict, PyString::NewInstance("__package__"),
                PyString::NewInstance(""));
  } else {
    std::string parent_name = JoinModuleName(parts, part_index);
    PyDict::Put(
        module_dict, PyString::NewInstance("__package__"),
        PyString::NewInstance(parent_name.c_str(),
                              static_cast<int64_t>(parent_name.size())));
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

ModuleExecutor::ModuleExecutor(Isolate* isolate, ModuleFinder* finder)
    : isolate_(isolate), finder_(finder) {}

Handle<PyObject> ModuleExecutor::LoadModulePart(
    Handle<PyDict> modules_dict,
    Handle<PyString> fullname,
    const std::vector<std::string>& parts,
    size_t part_index,
    const std::vector<std::string>& search_paths,
    BuiltinModuleRegistry* builtin_registry,
    ModuleManager* manager) {
  EscapableHandleScope scope;

  BuiltinModuleInitFunc builtin_init = nullptr;
  if (builtin_registry != nullptr) {
    builtin_init = builtin_registry->Find(ToStdString(fullname));
  }
  if (builtin_init != nullptr) {
    Handle<PyModule> module = builtin_init(isolate_, manager);
    return scope.Escape(Handle<PyObject>(module));
  }

  std::vector<std::string> relative_parts;
  if (part_index == 0) {
    relative_parts = std::vector<std::string>(parts.begin(), parts.begin() + 1);
  } else {
    relative_parts = std::vector<std::string>{parts[part_index]};
  }

  ModuleLocation loc =
      finder_->FindModuleLocation(search_paths, relative_parts);
  if (loc.origin.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: No module named '%s'\n",
                 ToStdString(fullname).c_str());
    std::exit(1);
  }

  std::string source;
  if (!finder_->ReadModuleSource(loc, &source)) {
    std::fprintf(stderr, "ImportError: cannot read '%s'\n", loc.origin.c_str());
    std::exit(1);
  }

  Handle<PyModule> module = PyModule::NewInstance();
  InitializeModuleDict(module, fullname, parts, part_index, loc);

  Handle<PyObject> module_obj(module);
  PyDict::Put(modules_dict, fullname, module_obj);

  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);
  Handle<PyFunction> boilerplate = Compiler::CompileSource(
      isolate_, std::string_view(source), std::string_view(loc.origin));
  boilerplate->set_func_globals(module_dict);
  isolate_->interpreter()->CallPython(boilerplate, Handle<PyObject>::null(),
                                      Handle<PyTuple>::null(),
                                      Handle<PyDict>::null(), module_dict);

  return scope.Escape(module_obj);
}

}  // namespace saauso::internal
