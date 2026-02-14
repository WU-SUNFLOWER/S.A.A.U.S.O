// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_FINDER_H_
#define SAAUSO_MODULES_MODULE_FINDER_H_

#include <string>
#include <vector>

namespace saauso::internal {

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

  ModuleLocation FindModuleLocation(
      const std::vector<std::string>& search_paths,
      const std::vector<std::string>& relative_parts) const;

  bool ReadModuleSource(const ModuleLocation& location, std::string* out) const;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_FINDER_H_

