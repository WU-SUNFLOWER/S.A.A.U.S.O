// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-finder.h"

#include <filesystem>

#include "src/execution/isolate.h"
#include "src/modules/module-utils.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/utils/file-utils.h"

namespace saauso::internal {

Maybe<ModuleLocation> ModuleFinder::FindModuleLocation(
    Handle<PyList> search_path_list,
    Handle<PyString> relative_name) const {
  HandleScope scope;
  ModuleLocation result;
  if (search_path_list.is_null()) {
    return Maybe<ModuleLocation>(result);
  }

  std::filesystem::path relative{relative_name->ToStdString()};

  for (int64_t i = 0; i < search_path_list->length(); ++i) {
    Handle<PyObject> elem = search_path_list->Get(i);
    if (elem.is_null()) {
      continue;
    }
    if (!IsPyString(elem)) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "sys.path items must be strings");
      return kNullMaybe;
    }
    std::string base = Handle<PyString>::cast(elem)->ToStdString();
    std::filesystem::path base_path(base);

#if SAAUSO_ENABLE_CPYTHON_COMPILER
    std::filesystem::path package_init = base_path / relative / "__init__.py";
    if (FileExists(package_init.string())) {
      result.origin = NormalizePath(package_init.string());
      result.kind = ModuleFileKind::kSourcePy;
      result.is_package = true;
      result.package_dir = NormalizePath((base_path / relative).string());
      return Maybe<ModuleLocation>(result);
    }
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

    std::filesystem::path package_init_pyc =
        base_path / relative / "__init__.pyc";
    if (FileExists(package_init_pyc.string())) {
      result.origin = NormalizePath(package_init_pyc.string());
      result.kind = ModuleFileKind::kBytecodePyc;
      result.is_package = true;
      result.package_dir = NormalizePath((base_path / relative).string());
      return Maybe<ModuleLocation>(result);
    }

#if SAAUSO_ENABLE_CPYTHON_COMPILER
    std::filesystem::path module_py = base_path / relative;
    module_py += ".py";
    if (FileExists(module_py.string())) {
      result.origin = NormalizePath(module_py.string());
      result.kind = ModuleFileKind::kSourcePy;
      result.is_package = false;
      return Maybe<ModuleLocation>(result);
    }
#endif  // SAAUSO_ENABLE_CPYTHON_COMPILER

    std::filesystem::path module_pyc = base_path / relative;
    module_pyc += ".pyc";
    if (FileExists(module_pyc.string())) {
      result.origin = NormalizePath(module_pyc.string());
      result.kind = ModuleFileKind::kBytecodePyc;
      result.is_package = false;
      return Maybe<ModuleLocation>(result);
    }
  }

  return Maybe<ModuleLocation>(result);
}

bool ModuleFinder::ReadModuleSource(const ModuleLocation& location,
                                    Handle<PyString>& out) const {
  if (location.origin.empty() || location.kind != ModuleFileKind::kSourcePy) {
    return false;
  }

  std::string raw_out;
  if (!ReadFileToString(location.origin, &raw_out)) {
    return false;
  }

  out = ModuleUtils::NewPyString(raw_out);
  return true;
}

}  // namespace saauso::internal
