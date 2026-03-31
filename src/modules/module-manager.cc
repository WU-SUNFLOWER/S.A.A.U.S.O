// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-manager.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/modules/builtin-module-registry.h"
#include "src/modules/module-finder.h"
#include "src/modules/module-importer.h"
#include "src/modules/module-loader.h"
#include "src/modules/module-name-resolver.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/visitors.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////////////////////

ModuleManager::ModuleManager(Isolate* isolate) : isolate_(isolate) {
  builtin_registry_ = std::make_unique<BuiltinModuleRegistry>(isolate_);
  finder_ = std::make_unique<ModuleFinder>(isolate_);
  loader_ = std::make_unique<ModuleLoader>(isolate_, finder_.get(), this,
                                           builtin_registry_.get());
  importer_ = std::make_unique<ModuleImporter>(this, isolate_);
  name_resolver_ = std::make_unique<ModuleNameResolver>(isolate_);

  InitializeSysState();
  builtin_registry_->BootstrapAllBuiltinModules();
}

ModuleManager::~ModuleManager() = default;

void ModuleManager::Iterate(ObjectVisitor* v) {
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&modules_));
  v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&path_));
  builtin_registry_->Iterate(v);
}

Handle<PyDict> ModuleManager::modules() const {
  return handle(modules_tagged(), isolate_);
}

Tagged<PyDict> ModuleManager::modules_tagged() const {
  return Tagged<PyDict>::cast(modules_);
}

Handle<PyList> ModuleManager::path() const {
  return handle(path_tagged(), isolate_);
}

Tagged<PyList> ModuleManager::path_tagged() const {
  return Tagged<PyList>::cast(path_);
}

void ModuleManager::InitializeSysState() {
  HandleScope scope(isolate_);

  Handle<PyDict> modules = PyDict::New(isolate_);
  modules_ = *modules;

  Handle<PyList> path = PyList::New(isolate_);
  PyList::Append(path, ST(dot, isolate_), isolate_);
  path_ = *path;
}

MaybeHandle<PyModule> ModuleManager::ImportModule(Handle<PyString> name,
                                                  Handle<PyTuple> fromlist,
                                                  int64_t level,
                                                  Handle<PyDict> globals) {
  return importer_->ImportModule(name, fromlist, level, globals);
}

}  // namespace saauso::internal
