// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-importer.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "src/modules/builtin-module-registry.h"
#include "src/modules/module-executor.h"
#include "src/modules/module-manager.h"
#include "src/modules/module-resolver.h"
#include "src/modules/module-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

namespace {

std::vector<std::string> ExtractSearchPaths(Handle<PyList> path_list) {
  std::vector<std::string> result;
  if (path_list.is_null()) {
    return result;
  }
  result.reserve(static_cast<size_t>(path_list->length()));
  for (int64_t i = 0; i < path_list->length(); ++i) {
    Handle<PyObject> elem = path_list->Get(i);
    if (elem.is_null()) {
      continue;
    }
    if (!IsPyString(elem)) {
      std::fprintf(stderr, "TypeError: sys.path items must be strings\n");
      std::exit(1);
    }
    result.emplace_back(ModuleUtils::ToStdString(Handle<PyString>::cast(elem)));
  }
  return result;
}

Handle<PyList> GetPackagePathListOrDie(Handle<PyObject> module) {
  Handle<PyList> path = ModuleUtils::GetPackagePathList(module);
  if (path.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: parent is not a package\n");
    std::exit(1);
  }
  return path;
}

Handle<PyObject> ApplyImportReturnSemantics(
    Handle<PyDict> modules_dict,
    const std::vector<std::string>& parts,
    Handle<PyTuple> fromlist,
    Handle<PyObject> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  if (!has_fromlist && parts.size() > 1) {
    Handle<PyString> top_name = PyString::NewInstance(
        parts[0].c_str(), static_cast<int64_t>(parts[0].size()));
    return modules_dict->Get(top_name);
  }
  return last_module;
}

void BindChildToParent(Handle<PyDict> modules_dict,
                       std::string_view parent_fullname,
                       std::string_view child_short_name,
                       Handle<PyObject> child) {
  Handle<PyString> parent_name = PyString::NewInstance(
      parent_fullname.data(), static_cast<int64_t>(parent_fullname.size()));
  Handle<PyObject> parent = modules_dict->Get(parent_name);
  if (parent.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: missing parent module '%.*s'\n",
                 static_cast<int>(parent_fullname.size()),
                 parent_fullname.data());
    std::exit(1);
  }

  Handle<PyDict> parent_dict = PyObject::GetProperties(parent);
  PyDict::Put(
      parent_dict,
      PyString::NewInstance(child_short_name.data(),
                            static_cast<int64_t>(child_short_name.size())),
      child);
}

Handle<PyObject> ImportDottedName(ModuleManager* manager,
                                  BuiltinModuleRegistry* builtin_registry,
                                  Handle<PyDict> modules_dict,
                                  const std::vector<std::string>& parts) {
  Handle<PyObject> last_module;

  for (size_t i = 0; i < parts.size(); ++i) {
    std::string part_name = ModuleUtils::JoinModuleName(parts, i + 1);
    Handle<PyString> part_name_obj = PyString::NewInstance(
        part_name.c_str(), static_cast<int64_t>(part_name.size()));

    Handle<PyObject> cached = modules_dict->Get(part_name_obj);
    if (!cached.is_null()) {
      last_module = cached;
    } else {
      std::vector<std::string> search_paths;
      if (i == 0) {
        search_paths = ExtractSearchPaths(manager->path());
      } else {
        search_paths = ExtractSearchPaths(GetPackagePathListOrDie(last_module));
      }

      Handle<PyObject> loaded = manager->executor()->LoadModulePart(
          modules_dict, part_name_obj, parts, i, search_paths, builtin_registry,
          manager);

      PyDict::Put(modules_dict, part_name_obj, loaded);
      last_module = loaded;

      if (i > 0) {
        std::string parent_name = ModuleUtils::JoinModuleName(parts, i);
        BindChildToParent(modules_dict, parent_name, parts[i], loaded);
      }
    }

    if (i + 1 < parts.size() && !ModuleUtils::IsPackageModule(last_module)) {
      std::fprintf(stderr, "ImportError: '%s' is not a package\n",
                   part_name.c_str());
      std::exit(1);
    }
  }

  return last_module;
}

}  // namespace

Handle<PyObject> ModuleImporter::ImportModule(ModuleManager* manager,
                                              Handle<PyString> name,
                                              Handle<PyTuple> fromlist,
                                              int64_t level,
                                              Handle<PyDict> globals) {
  EscapableHandleScope scope;

  std::string fullname = ModuleResolver::ResolveFullName(name, level, globals);
  std::vector<std::string> parts = ModuleUtils::SplitModuleName(fullname);
  if (parts.empty()) {
    std::fprintf(stderr, "ModuleNotFoundError: invalid module name '%s'\n",
                 fullname.c_str());
    std::exit(1);
  }

  Handle<PyDict> modules_dict = manager->modules();
  Handle<PyObject> last_module = ImportDottedName(
      manager, manager->builtin_registry_.get(), modules_dict, parts);
  Handle<PyObject> result =
      ApplyImportReturnSemantics(modules_dict, parts, fromlist, last_module);

  return scope.Escape(result);
}

}  // namespace saauso::internal
