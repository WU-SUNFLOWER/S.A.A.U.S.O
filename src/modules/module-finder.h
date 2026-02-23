// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_FINDER_H_
#define SAAUSO_MODULES_MODULE_FINDER_H_

#include <string>

#include "src/handles/handles.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

class Isolate;
class PyList;
class PyString;

enum class ModuleFileKind {
  kNotFound = 0,
  kSourcePy,
  kBytecodePyc,
};

struct ModuleLocation {
  std::string origin;
  ModuleFileKind kind{ModuleFileKind::kNotFound};
  bool is_package{false};
  std::string package_dir;
};

class ModuleFinder final {
 public:
  ModuleFinder() = default;

  ModuleFinder(const ModuleFinder&) = delete;
  ModuleFinder& operator=(const ModuleFinder&) = delete;
  ~ModuleFinder() = default;

  // 失败时（如 sys.path 项非字符串）设置 pending exception 并返回 nullopt。
  Maybe<ModuleLocation> FindModuleLocation(
      Handle<PyList> search_path_list,
      Handle<PyString> relative_name) const;

  bool ReadModuleSource(const ModuleLocation& location,
                        Handle<PyString>& out) const;
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_FINDER_H_
