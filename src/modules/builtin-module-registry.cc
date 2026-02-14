// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/builtin-module-registry.h"

namespace saauso::internal {

void BuiltinModuleRegistry::Register(std::string_view name,
                                     BuiltinModuleInitFunc init) {
  Entry entry;
  entry.name = std::string(name);
  entry.init = init;
  entries_.push_back(std::move(entry));
}

BuiltinModuleInitFunc BuiltinModuleRegistry::Find(std::string_view name) const {
  for (const auto& entry : entries_) {
    if (entry.name == name) {
      return entry.init;
    }
  }
  return nullptr;
}

}  // namespace saauso::internal

