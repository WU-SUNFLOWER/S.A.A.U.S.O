// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-resolver.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "src/modules/module-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Handle<PyString> ModuleResolver::ResolveFullName(Handle<PyString> name,
                                                 int64_t level,
                                                 Handle<PyDict> globals) {
  EscapableHandleScope scope;
  assert(level >= 0);

  // 只有在相对导入的情况下，才允许模块name为空，
  // 例如`from .. import my_module`
  if (level == 0 && (name.is_null() || name->length() == 0)) {
    std::fprintf(stderr, "ModuleNotFoundError: empty module name\n");
    std::exit(1);
  }

  Handle<PyString> fullname = ResolveRelativeImportName(name, level, globals);
  return scope.Escape(fullname);
}

Handle<PyString> ModuleResolver::ResolveRelativeImportName(
    Handle<PyString> name,
    int64_t level,
    Handle<PyDict> globals) {
  EscapableHandleScope scope;

  // 如果不是相对导入，自然不用处理了，直接返回即可
  if (level <= 0) {
    if (name.is_null()) {
      return scope.Escape(PyString::NewInstance(""));
    }
    return scope.Escape(name);
  }

  Handle<PyString> base = ResolvePackageFromGlobals(globals);
  if (base->length() == 0) {
    std::fprintf(stderr,
                 "ImportError: attempted relative import with no known parent "
                 "package\n");
    std::exit(1);
  }

  for (int i = 1; i < level; ++i) {
    Handle<PyString> parent = ParentModuleNameOrEmpty(base);
    if (parent->length() == 0) {
      std::fprintf(
          stderr,
          "ImportError: attempted relative import beyond top-level package\n");
      std::exit(1);
    }
    base = parent;
  }

  if (name.is_null() || name->length() == 0) {
    return scope.Escape(base);
  }

  Handle<PyString> fullname = PyString::Append(base, ST(dot));
  fullname = PyString::Append(fullname, name);
  return scope.Escape(fullname);
}

Handle<PyString> ModuleResolver::ResolvePackageFromGlobals(
    Handle<PyDict> globals) {
  EscapableHandleScope scope;

  if (globals.is_null()) {
    return scope.Escape(PyString::NewInstance(""));
  }

  Handle<PyObject> package_obj = globals->Get(ST(package));
  if (!package_obj.is_null()) {
    if (!IsPyString(package_obj)) {
      std::fprintf(stderr, "TypeError: __package__ must be a string\n");
      std::exit(1);
    }
    return scope.Escape(Handle<PyString>::cast(package_obj));
  }

  Handle<PyObject> name_obj = globals->Get(ST(name));
  if (name_obj.is_null() || !IsPyString(name_obj)) {
    std::fprintf(stderr, "ImportError: missing __name__ in globals\n");
    std::exit(1);
  }

  Handle<PyString> name_str = Handle<PyString>::cast(name_obj);
  bool is_package = !globals->Get(ST(path)).is_null();
  if (is_package) {
    return scope.Escape(name_str);
  }
  return scope.Escape(ParentModuleNameOrEmpty(name_str));
}

Handle<PyString> ModuleResolver::ParentModuleNameOrEmpty(
    Handle<PyString> name) {
  EscapableHandleScope scope;

  if (name.is_null() || name->length() == 0) {
    return scope.Escape(PyString::NewInstance(""));
  }

  std::string_view name_view = ModuleUtils::ToStringView(name);
  size_t dot = name_view.rfind('.');
  if (dot == std::string_view::npos || dot == 0) {
    return scope.Escape(PyString::NewInstance(""));
  }

  int64_t parent_end = static_cast<int64_t>(dot) - 1;
  return scope.Escape(PyString::Slice(name, 0, parent_end));
}

}  // namespace saauso::internal
