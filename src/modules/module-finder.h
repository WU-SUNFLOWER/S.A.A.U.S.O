// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_FINDER_H_
#define SAAUSO_MODULES_MODULE_FINDER_H_

#include <string>
#include <string_view>

#include "src/handles/handles.h"

namespace saauso::internal {

class PyList;
class PyString;

struct ModuleLocation {
  std::string origin;
  bool is_package{false};
  std::string package_dir;
};

class ModuleFinder final {
 public:
  ModuleFinder() = default;
  ModuleFinder(const ModuleFinder&) = delete;
  ModuleFinder& operator=(const ModuleFinder&) = delete;
  ~ModuleFinder() = default;

  ModuleLocation FindModuleLocation(Handle<PyList> search_path_list,
                                    std::string_view relative_name) const;

  bool ReadModuleSource(const ModuleLocation& location, Handle<PyString>& out) const;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_FINDER_H_
