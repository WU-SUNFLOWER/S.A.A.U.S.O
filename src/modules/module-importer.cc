// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-importer.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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

Handle<PyObject> ApplyImportReturnSemantics(Handle<PyDict> modules_dict,
                                            std::string_view fullname,
                                            Handle<PyTuple> fromlist,
                                            Handle<PyObject> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  size_t dot = fullname.find('.');
  if (!has_fromlist && dot != std::string_view::npos) {
    Handle<PyString> top_name =
        PyString::NewInstance(fullname.data(), static_cast<int64_t>(dot));
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

bool IsValidModuleName(std::string_view fullname) {
  if (fullname.empty()) {
    return false;
  }
  if (fullname.front() == '.' || fullname.back() == '.') {
    return false;
  }
  for (size_t i = 1; i < fullname.size(); ++i) {
    if (fullname[i] == '.' && fullname[i - 1] == '.') {
      return false;
    }
  }
  return true;
}

}  // namespace

////////////////////////////////////////////////////////////////////////

ModuleImporter::ModuleImporter(ModuleManager* manager) : manager_(manager) {}

Handle<PyObject> ModuleImporter::ImportModule(Handle<PyString> name,
                                              Handle<PyTuple> fromlist,
                                              int64_t level,
                                              Handle<PyDict> globals) {
  EscapableHandleScope scope;

  std::string fullname = ModuleResolver::ResolveFullName(name, level, globals);
  if (!IsValidModuleName(fullname)) {
    std::fprintf(stderr, "ModuleNotFoundError: invalid module name '%s'\n",
                 fullname.c_str());
    std::exit(1);
  }

  Handle<PyDict> modules_dict = manager_->modules();
  Handle<PyObject> last_module =
      LinkAndImportModuleImpl(modules_dict, std::string_view(fullname));
  Handle<PyObject> result = ApplyImportReturnSemantics(
      modules_dict, std::string_view(fullname), fromlist, last_module);

  return scope.Escape(result);
}

Handle<PyObject> ModuleImporter::LinkAndImportModuleImpl(
    Handle<PyDict> modules_dict,
    std::string_view fullname) {
  Handle<PyObject> last_module;

  size_t segment_start = 0;
  while (segment_start <= fullname.size()) {
    size_t dot = fullname.find('.', segment_start);
    if (dot == std::string_view::npos) {
      dot = fullname.size();
    }

    std::string_view segment =
        fullname.substr(segment_start, dot - segment_start);
    Handle<PyString> part_name_obj =
        PyString::NewInstance(fullname.data(), static_cast<int64_t>(dot));
    Handle<PyObject> cached = modules_dict->Get(part_name_obj);
    if (!cached.is_null()) {
      last_module = cached;
    } else {
      std::vector<std::string> search_paths;
      if (segment_start == 0) {
        search_paths = ExtractSearchPaths(manager_->path());
      } else {
        search_paths = ExtractSearchPaths(GetPackagePathListOrDie(last_module));
      }

      Handle<PyObject> loaded =
          manager_->executor()->LoadModulePart(part_name_obj, search_paths);
      last_module = loaded;

      if (segment_start != 0) {
        std::string_view parent_fullname =
            fullname.substr(0, segment_start - 1);
        BindChildToParent(modules_dict, parent_fullname, segment, loaded);
      }
    }

    if (dot != fullname.size() && !ModuleUtils::IsPackageModule(last_module)) {
      std::fprintf(stderr, "ImportError: '%s' is not a package\n",
                   std::string(fullname.substr(0, dot)).c_str());
      std::exit(1);
    }

    if (dot == fullname.size()) {
      break;
    }
    segment_start = dot + 1;
  }

  return last_module;
}

}  // namespace saauso::internal
