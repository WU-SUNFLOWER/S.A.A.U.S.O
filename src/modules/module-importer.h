// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_IMPORTER_H_
#define SAAUSO_MODULES_MODULE_IMPORTER_H_

#include <cstdint>

#include "src/handles/handles.h"

namespace saauso::internal {

class ModuleManager;
class PyDict;
class PyObject;
class PyString;
class PyTuple;

class ModuleImporter final {
 public:
  ModuleImporter() = default;
  ModuleImporter(const ModuleImporter&) = delete;
  ModuleImporter& operator=(const ModuleImporter&) = delete;
  ~ModuleImporter() = default;

  Handle<PyObject> ImportModule(ModuleManager* manager,
                                Handle<PyString> name,
                                Handle<PyTuple> fromlist,
                                int64_t level,
                                Handle<PyDict> globals);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_IMPORTER_H_

