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
  ModuleImporter(ModuleManager* manager);
  ModuleImporter(const ModuleImporter&) = delete;
  ModuleImporter& operator=(const ModuleImporter&) = delete;
  ~ModuleImporter() = default;

  Handle<PyObject> ImportModule(Handle<PyString> name,
                                Handle<PyTuple> fromlist,
                                int64_t level,
                                Handle<PyDict> globals);

 private:
  Handle<PyObject> ImportModuleImpl(Handle<PyString> fullname);

  void LinkChildToParent(Handle<PyString> parent_part_name,
                         Handle<PyString> fullname,
                         int64_t child_begin,
                         int64_t child_end,
                         Handle<PyObject> child);
  void LinkChildToParentImpl(Handle<PyString> parent_name,
                             Handle<PyString> child_short_name,
                             Handle<PyObject> child);

Handle<PyObject> ApplyImportReturnSemantics(
                                            Handle<PyString> fullname,
                                            Handle<PyTuple> fromlist,
                                            Handle<PyObject> last_module);

  Handle<PyDict> modules_dict();

  ModuleManager* manager_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_IMPORTER_H_
