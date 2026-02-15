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
#include "src/objects/py-module.h"
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

Handle<PyModule> ModuleImporter::ImportModule(Handle<PyString> name,
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

  Handle<PyModule> last_module = ImportModuleImpl(fullname);
  Handle<PyModule> result =
      ApplyImportReturnSemantics(fullname, fromlist, last_module);

  return scope.Escape(result);
}

Handle<PyModule> ModuleImporter::ImportModuleImpl(Handle<PyString> fullname) {
  Handle<PyModule> last_module;

  int64_t segment_start = 0;
  while (true) {
    ModulePartScanResult scan = ScanNextPart(fullname, segment_start);

    Handle<PyString> part_module_fullname =
        PyString::Slice(fullname, 0, scan.part_end);
    Handle<PyModule> part_module = GetOrLoadModulePart(
        part_module_fullname, segment_start == 0, last_module);

    if (segment_start != 0) {
      Handle<PyString> part_module_short_name =
          PyString::Slice(fullname, segment_start, scan.part_end);
      BindChildModuleToParentNamespace(last_module, part_module_short_name,
                                       part_module);
    }

    last_module = part_module;
    if (scan.is_last) {
      break;
    }

    // 如果当前段不是最后一段，则对应 module 必须为 package
    EnsurePackageForNextSegment(part_module, part_module_fullname);

    segment_start = scan.dot + 1;
  }

  return last_module;
}

Handle<PyModule> ModuleImporter::GetOrLoadModulePart(
    Handle<PyString> part_fullname,
    bool is_top,
    Handle<PyObject> parent_module) {
  // Fast Path: 模块已经被缓存
  Handle<PyObject> cached = modules_dict()->Get(part_fullname);
  if (!cached.is_null()) {
    return Handle<PyModule>::cast(cached);
  }

  // Slow Path: 走module loader加载模块
  Handle<PyList> search_path_list = SelectSearchPathList(is_top, parent_module);
  return manager_->loader()->LoadModulePart(part_fullname, search_path_list);
}

Handle<PyList> ModuleImporter::SelectSearchPathList(
    bool is_top,
    Handle<PyObject> parent_module) {
  // 顶层模块直接走sys.path进行查找
  if (is_top) {
    assert(parent_module.is_null());
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

  // 如果父模块中已经链接了子模块，则不需要重复链接。
  // 但应确保该子模块与当前准备链接的child_module是同一个module object对象。
  if (parent_dict->Contains(child_short_name)) {
    assert(parent_dict->Get(child_short_name).is_identical_to(child_module));
    return;
  }

  PyDict::Put(parent_dict, child_short_name, child_module);
}

void ModuleImporter::EnsurePackageForNextSegment(
    Handle<PyObject> module,
    Handle<PyString> module_fullname) {
  if (!ModuleUtils::IsPackageModule(module)) {
    std::fprintf(stderr, "ImportError: '%s' is not a package\n",
                 module_fullname->buffer());
    std::exit(1);
  }
}

Handle<PyModule> ModuleImporter::ApplyImportReturnSemantics(
    Handle<PyString> fullname,
    Handle<PyTuple> fromlist,
    Handle<PyModule> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  int64_t dot = fullname->IndexOf(ST(dot));
  if (!has_fromlist && dot != PyString::kNotFound) {
    int64_t top_end = dot - 1;
    Handle<PyString> top_name = PyString::Slice(fullname, 0, top_end);
    return Handle<PyModule>::cast(modules_dict()->Get(top_name));
  }
  return last_module;
}

Handle<PyDict> ModuleImporter::modules_dict() {
  return manager_->modules();
}

}  // namespace saauso::internal
