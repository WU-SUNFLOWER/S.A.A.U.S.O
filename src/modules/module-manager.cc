// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-manager.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/modules/builtin-module-registry.h"
#include "src/modules/module-executor.h"
#include "src/modules/module-finder.h"
#include "src/modules/module-importer.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

namespace {

Handle<PyModule> InitSysModule(Isolate* isolate, ModuleManager* manager) {
  EscapableHandleScope scope;

  Handle<PyModule> module = PyModule::NewInstance();
  Handle<PyObject> module_obj(module);
  Handle<PyDict> module_dict = PyObject::GetProperties(module_obj);

  PyDict::Put(module_dict, PyString::NewInstance("__name__"),
              PyString::NewInstance("sys"));
  PyDict::Put(module_dict, PyString::NewInstance("__package__"),
              PyString::NewInstance(""));
  PyDict::Put(module_dict, PyString::NewInstance("modules"),
              manager->modules());
  PyDict::Put(module_dict, PyString::NewInstance("path"), manager->path());
  PyDict::Put(module_dict, PyString::NewInstance("version"),
              PyString::NewInstance("3.12 (saauso mvp)"));

  return scope.Escape(module);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////////

ModuleManager::ModuleManager(Isolate* isolate) : isolate_(isolate) {
  builtin_registry_ = std::make_unique<BuiltinModuleRegistry>();
  finder_ = std::make_unique<ModuleFinder>();
  executor_ = std::make_unique<ModuleExecutor>(isolate_, finder_.get());
  importer_ = std::make_unique<ModuleImporter>();
  InitializeSysState();
  RegisterBuiltinModules();
}

ModuleManager::~ModuleManager() = default;

void ModuleManager::Iterate(ObjectVisitor* v) {
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&modules_));
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&path_));
}

Handle<PyDict> ModuleManager::modules() const {
  return handle(modules_tagged());
}

Tagged<PyDict> ModuleManager::modules_tagged() const {
  return Tagged<PyDict>::cast(modules_);
}

Handle<PyList> ModuleManager::path() const {
  return handle(path_tagged());
}

Tagged<PyList> ModuleManager::path_tagged() const {
  return Tagged<PyList>::cast(path_);
}

void ModuleManager::InitializeSysState() {
  HandleScope scope;

  Handle<PyDict> modules = PyDict::NewInstance();
  modules_ = *modules;

  Handle<PyList> path = PyList::NewInstance();
  PyList::Append(path, PyString::NewInstance("."));
  path_ = *path;
}

BuiltinModuleInitFunc ModuleManager::FindBuiltinModule(
    std::string_view name) const {
  return builtin_registry_->Find(name);
}

void ModuleManager::RegisterBuiltinModules() {
  builtin_registry_->Register("sys", &InitSysModule);
}

Handle<PyObject> ModuleManager::ImportModule(Handle<PyString> name,
                                             Handle<PyTuple> fromlist,
                                             int64_t level,
                                             Handle<PyDict> globals) {
  return importer_->ImportModule(this, name, fromlist, level, globals);
}

}  // namespace saauso::internal
