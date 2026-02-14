// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-resolver.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "src/modules/module-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

std::string ModuleResolver::ResolveFullName(Handle<PyString> name,
                                            int64_t level,
                                            Handle<PyDict> globals) {
  assert(level >= 0);

  std::string name_str = ModuleUtils::ToStdString(name);

  // 只有在相对导入的情况下，才允许模块name为空，
  // 例如`from .. import my_module`
  if (level == 0 && name_str.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: empty module name\n");
    std::exit(1);
  }

  return ResolveRelativeImportName(name_str, level, globals);
}

std::string ModuleResolver::ResolveRelativeImportName(std::string_view name,
                                                      int64_t level,
                                                      Handle<PyDict> globals) {
  // 如果不是相对导入，自然不用处理了，直接返回即可
  if (level <= 0) {
    return std::string(name);
  }

  std::string base = ResolvePackageFromGlobals(globals);
  if (base.empty()) {
    std::fprintf(stderr,
                 "ImportError: attempted relative import with no known parent "
                 "package\n");
    std::exit(1);
  }

  for (int i = 1; i < level; ++i) {
    std::string parent = ParentModuleNameOrEmpty(base);
    if (parent.empty()) {
      std::fprintf(
          stderr,
          "ImportError: attempted relative import beyond top-level package\n");
      std::exit(1);
    }
    base = std::move(parent);
  }

  if (name.empty()) {
    return base;
  }

  std::string fullname = base;
  fullname.push_back('.');
  fullname.append(name);
  return fullname;
}

std::string ModuleResolver::ResolvePackageFromGlobals(Handle<PyDict> globals) {
  if (globals.is_null()) {
    return std::string();
  }

  Handle<PyObject> package_obj = globals->Get(ST(package));
  if (!package_obj.is_null()) {
    if (!IsPyString(package_obj)) {
      std::fprintf(stderr, "TypeError: __package__ must be a string\n");
      std::exit(1);
    }
    return ModuleUtils::ToStdString(Handle<PyString>::cast(package_obj));
  }

  Handle<PyObject> name_obj = globals->Get(ST(name));
  if (name_obj.is_null() || !IsPyString(name_obj)) {
    std::fprintf(stderr, "ImportError: missing __name__ in globals\n");
    std::exit(1);
  }

  std::string name_str =
      ModuleUtils::ToStdString(Handle<PyString>::cast(name_obj));
  bool is_package = !globals->Get(ST(path)).is_null();
  if (is_package) {
    return name_str;
  }
  return ParentModuleNameOrEmpty(name_str);
}

std::string ModuleResolver::ParentModuleNameOrEmpty(std::string_view name) {
  size_t dot = name.rfind('.');
  if (dot == std::string_view::npos) {
    return std::string();
  }
  return std::string(name.substr(0, dot));
}

}  // namespace saauso::internal
