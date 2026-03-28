// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/builtin-module-registry.h"

#include "src/execution/isolate.h"
#include "src/objects/py-string.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

BUILTIN_MODULE_LIST(DECL_BUILTIN_MODULE_INIT_FUNC);

void BuiltinModuleRegistry::Register(Handle<PyString> name,
                                     BuiltinModuleInitFunc init) {
  Entry entry;
  entry.name = *name;
  entry.init = init;
  entries_.PushBack(std::move(entry));
}

void BuiltinModuleRegistry::BootstrapAllBuiltinModules() {
  HandleScope scope;

#define REGISTER_BUILTIN_MODULE(module_name, func_name) \
  Register(PyString::New(Isolate::Current(), module_name), &func_name);
  BUILTIN_MODULE_LIST(REGISTER_BUILTIN_MODULE)
#undef REGISTER_BUILTIN_MODULE
}

BuiltinModuleInitFunc BuiltinModuleRegistry::Find(Handle<PyString> name) const {
  for (size_t i = 0; i < entries_.length(); ++i) {
    const auto& entry = entries_.Get(i);
    if (name->IsEqualTo(entry.name)) {
      return entry.init;
    }
  }

  return nullptr;
}

void BuiltinModuleRegistry::Iterate(ObjectVisitor* v) {
  for (size_t i = 0; i < entries_.length(); ++i) {
    auto& entry = entries_.Get(i);
    v->VisitPointer(reinterpret_cast<Tagged<PyObject>*>(&entry.name));
  }
}

}  // namespace saauso::internal
