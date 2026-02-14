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

struct ModulePartScanResult {
  int64_t dot{0};
  int64_t part_end{0};
  bool is_last{false};
};

ModulePartScanResult ScanNextPart(Handle<PyString> fullname,
                                  int64_t segment_start) {
  int64_t fullname_len = fullname->length();
  int64_t dot = fullname->IndexOf(ST(dot), segment_start, fullname_len);
  if (dot == PyString::kNotFound) {
    dot = fullname_len;
  }

  ModulePartScanResult result;
  result.dot = dot;
  result.part_end = dot - 1;
  result.is_last = dot == fullname_len;
  return result;
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
  Handle<PyObject> parent_module;
  Handle<PyObject> last_module;

  int64_t segment_start = 0;
  while (true) {
    ModulePartScanResult scan = ScanNextPart(fullname, segment_start);

    Handle<PyString> part_fullname =
        PyString::Slice(fullname, 0, scan.part_end);
    Handle<PyObject> module =
        GetOrLoadModulePart(part_fullname, segment_start == 0, parent_module);

    if (segment_start != 0) {
      Handle<PyString> child_short_name =
          PyString::Slice(fullname, segment_start, scan.part_end);
      BindChildModuleToParentNamespace(parent_module, child_short_name, module);
    }

    EnsurePackageForNextSegment(module, fullname, scan.dot);

    last_module = module;
    if (scan.is_last) {
      break;
    }

    parent_module = module;
    segment_start = scan.dot + 1;
  }

  return last_module;
}

Handle<PyObject> ModuleImporter::GetOrLoadModulePart(
    Handle<PyString> part_fullname,
    bool is_top,
    Handle<PyObject> parent_module) {
  Handle<PyObject> cached = modules_dict()->Get(part_fullname);
  if (!cached.is_null()) {
    return cached;
  }

  Handle<PyList> search_path_list = SelectSearchPathList(is_top, parent_module);
  return manager_->loader()->LoadModulePart(part_fullname, search_path_list);
}

Handle<PyList> ModuleImporter::SelectSearchPathList(
    bool is_top,
    Handle<PyObject> parent_module) {
  if (is_top) {
    return manager_->path();
  }

  Handle<PyList> path = ModuleUtils::GetPackagePathList(parent_module);
  if (path.is_null()) [[unlikely]] {
    std::fprintf(stderr, "ImportError: parent is not a package\n");
    std::exit(1);
  }
  return path;
}

void ModuleImporter::BindChildModuleToParentNamespace(
    Handle<PyObject> parent_module,
    Handle<PyString> child_short_name,
    Handle<PyObject> child_module) {
  Handle<PyDict> parent_dict = PyObject::GetProperties(parent_module);
  PyDict::Put(parent_dict, child_short_name, child_module);
}

void ModuleImporter::EnsurePackageForNextSegment(Handle<PyObject> module,
                                                 Handle<PyString> fullname,
                                                 int64_t dot) {
  if (dot != fullname->length() && !ModuleUtils::IsPackageModule(module)) {
    std::fprintf(stderr, "ImportError: '%.*s' is not a package\n",
                 static_cast<int>(dot), fullname->buffer());
    std::exit(1);
  }
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
