// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-finder.h"

#include <filesystem>

#include "src/utils/file-utils.h"

namespace saauso::internal {

ModuleLocation ModuleFinder::FindModuleLocation(
    const std::vector<std::string>& search_paths,
    const std::vector<std::string>& relative_parts) const {
  ModuleLocation result;

  std::filesystem::path relative;
  for (const auto& part : relative_parts) {
    relative /= std::filesystem::path(part);
  }

  for (const auto& base : search_paths) {
    std::filesystem::path base_path(base);

    std::filesystem::path package_init = base_path / relative / "__init__.py";
    if (FileExists(package_init.string())) {
      result.origin = NormalizePath(package_init.string());
      result.is_package = true;
      result.package_dir = NormalizePath((base_path / relative).string());
      return result;
    }

    std::filesystem::path module_py = base_path / relative;
    module_py += ".py";
    if (FileExists(module_py.string())) {
      result.origin = NormalizePath(module_py.string());
      result.is_package = false;
      return result;
    }
  }

  return result;
}

bool ModuleFinder::ReadModuleSource(const ModuleLocation& location,
                                    std::string* out) const {
  if (location.origin.empty()) {
    return false;
  }
  return ReadFileToString(location.origin, out);
}

}  // namespace saauso::internal

