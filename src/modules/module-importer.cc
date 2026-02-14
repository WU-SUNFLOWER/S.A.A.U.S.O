// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-importer.h"

#include <cstdio>
#include <cstdlib>

#include "src/modules/module-loader.h"
#include "src/modules/module-manager.h"
#include "src/modules/module-name-resolver.h"
#include "src/modules/module-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

Handle<PyList> GetPackagePathListOrDie(Handle<PyObject> module) {
  Handle<PyList> path = ModuleUtils::GetPackagePathList(module);
  if (path.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: parent is not a package\n");
    std::exit(1);
  }
  return path;
}

}  // namespace

////////////////////////////////////////////////////////////////////////

ModuleImporter::ModuleImporter(ModuleManager* manager) : manager_(manager) {}

Handle<PyObject> ModuleImporter::ImportModule(Handle<PyString> name,
                                              Handle<PyTuple> fromlist,
                                              int64_t level,
                                              Handle<PyDict> globals) {
  EscapableHandleScope scope;

  Handle<PyString> fullname =
      ModuleNameResolver::ResolveFullName(name, level, globals);
  if (!ModuleUtils::IsValidModuleName(fullname)) {
    std::fprintf(stderr, "ModuleNotFoundError: invalid module name '%.*s'\n",
                 static_cast<int>(fullname->length()), fullname->buffer());
    std::exit(1);
  }

  Handle<PyObject> last_module = ImportModuleImpl(fullname);
  Handle<PyObject> result =
      ApplyImportReturnSemantics(fullname, fromlist, last_module);

  return scope.Escape(result);
}

Handle<PyObject> ModuleImporter::ImportModuleImpl(Handle<PyString> fullname) {
  Handle<PyObject> last_module;
  Handle<PyString> parent_part_name;

  int64_t segment_start = 0;
  int64_t fullname_len = fullname->length();
  while (segment_start <= fullname_len) {
    int64_t dot = fullname->IndexOf(ST(dot), segment_start, fullname_len);
    if (dot == PyString::kNotFound) {
      dot = fullname_len;
    }

    int64_t part_end = dot - 1;
    Handle<PyString> part_name_obj = PyString::Slice(fullname, 0, part_end);
    Handle<PyObject> cached = modules_dict()->Get(part_name_obj);
    if (!cached.is_null()) {
      last_module = cached;
    } else {
      Handle<PyList> search_path_list =
          segment_start == 0 ? manager_->path()
                             : GetPackagePathListOrDie(last_module);
      Handle<PyObject> loaded =
          manager_->loader()->LoadModulePart(part_name_obj, search_path_list);
      last_module = loaded;

      LinkChildToParent(parent_part_name, fullname, segment_start, dot - 1,
                        loaded);
    }

    if (dot != fullname_len && !ModuleUtils::IsPackageModule(last_module)) {
      std::fprintf(stderr, "ImportError: '%.*s' is not a package\n",
                   static_cast<int>(dot), fullname->buffer());
      std::exit(1);
    }

    if (dot == fullname_len) {
      break;
    }
    parent_part_name = part_name_obj;
    segment_start = dot + 1;
  }

  return last_module;
}

void ModuleImporter::LinkChildToParent(Handle<PyString> parent_part_name,
                                       Handle<PyString> fullname,
                                       int64_t child_begin,
                                       int64_t child_end,
                                       Handle<PyObject> child) {
  if (child_begin == 0) {
    return;
  }

  Handle<PyString> child_short_name_obj =
      PyString::Slice(fullname, child_begin, child_end);
  LinkChildToParentImpl(parent_part_name, child_short_name_obj, child);
}

void ModuleImporter::LinkChildToParentImpl(Handle<PyString> parent_name,
                                           Handle<PyString> child_short_name,
                                           Handle<PyObject> child) {
  Handle<PyObject> parent = modules_dict()->Get(parent_name);
  if (parent.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: missing parent module '%.*s'\n",
                 static_cast<int>(parent_name->length()),
                 parent_name->buffer());
    std::exit(1);
  }

  Handle<PyDict> parent_dict = PyObject::GetProperties(parent);
  PyDict::Put(parent_dict, child_short_name, child);
}

Handle<PyObject> ModuleImporter::ApplyImportReturnSemantics(
    Handle<PyString> fullname,
    Handle<PyTuple> fromlist,
    Handle<PyObject> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  int64_t dot = fullname->IndexOf(ST(dot));
  if (!has_fromlist && dot != PyString::kNotFound) {
    int64_t top_end = dot - 1;
    Handle<PyString> top_name = PyString::Slice(fullname, 0, top_end);
    return modules_dict()->Get(top_name);
  }
  return last_module;
}

Handle<PyDict> ModuleImporter::modules_dict() {
  return manager_->modules();
}

}  // namespace saauso::internal
