// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_RESOLVER_H_
#define SAAUSO_MODULES_MODULE_RESOLVER_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "src/handles/handles.h"

namespace saauso::internal {

class PyDict;
class PyString;

class ModuleResolver final {
 public:
  static std::string ResolveFullName(Handle<PyString> name,
                                     int64_t level,
                                     Handle<PyDict> globals);

 private:
  static std::string ParentModuleNameOrEmpty(std::string_view name);
  static std::string ResolvePackageFromGlobals(Handle<PyDict> globals);
  static std::string ResolveRelativeImportName(std::string_view name,
                                               int64_t level,
                                               Handle<PyDict> globals);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_RESOLVER_H_
