// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-importer.h"

#include "src/execution/exception-utils.h"
#include "src/modules/module-loader.h"
#include "src/modules/module-manager.h"
#include "src/modules/module-name-resolver.h"
#include "src/modules/module-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

namespace {

struct ModulePartScanResult {
  int64_t dot{0};
  int64_t part_end{0};
  bool is_last{false};
};

ModulePartScanResult ScanNextPart(Isolate* isolate,
                                  Handle<PyString> fullname,
                                  int64_t segment_start) {
  int64_t fullname_len = fullname->length();
  int64_t dot =
      fullname->IndexOf(ST(dot, isolate), segment_start, fullname_len);
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

ModuleImporter::ModuleImporter(ModuleManager* manager, Isolate* isolate)
    : manager_(manager), isolate_(isolate) {}

MaybeHandle<PyModule> ModuleImporter::ImportModule(Handle<PyString> name,
                                                   Handle<PyTuple> fromlist,
                                                   int64_t level,
                                                   Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate_);

  Handle<PyString> fullname;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, fullname,
      manager_->name_resolver()->ResolveFullName(name, level, globals));

  if (!ModuleUtils::IsValidModuleName(fullname)) {
    Runtime_ThrowErrorf(isolate_, ExceptionType::kModuleNotFoundError,
                        "invalid module name '%s'", fullname->buffer());
    return kNullMaybe;
  }

  Handle<PyModule> last_module;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, last_module, ImportModuleImpl(fullname));

  Handle<PyModule> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, result,
      ApplyImportReturnSemantics(fullname, fromlist, last_module));

  return scope.Escape(result);
}

MaybeHandle<PyModule> ModuleImporter::ImportModuleImpl(
    Handle<PyString> fullname) {
  Handle<PyModule> last_module;

  int64_t segment_start = 0;
  while (true) {
    ModulePartScanResult scan = ScanNextPart(isolate_, fullname, segment_start);

    Handle<PyString> part_module_fullname =
        PyString::Slice(fullname, 0, scan.part_end, isolate_);
    Handle<PyModule> part_module;
    ASSIGN_RETURN_ON_EXCEPTION(
        manager_->isolate(), part_module,
        GetOrLoadModulePart(part_module_fullname, segment_start == 0,
                            last_module));

    if (segment_start != 0) {
      Handle<PyString> part_module_short_name =
          PyString::Slice(fullname, segment_start, scan.part_end, isolate_);
      RETURN_ON_EXCEPTION(
          isolate_, BindChildModuleToParentNamespace(
                        last_module, part_module_short_name, part_module));
    }

    last_module = part_module;
    if (scan.is_last) {
      break;
    }

    // 如果当前段不是最后一段，则对应 module 必须为 package！
    // 否则抛出错误！
    RETURN_ON_EXCEPTION(isolate_, EnsurePackageForNextSegment(
                                      part_module, part_module_fullname));

    segment_start = scan.dot + 1;
  }

  return last_module;
}

MaybeHandle<PyModule> ModuleImporter::GetOrLoadModulePart(
    Handle<PyString> part_fullname,
    bool is_top,
    Handle<PyObject> parent_module) {
  // Fast Path: 模块已经被缓存
  Handle<PyObject> cached;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, found, modules_dict()->Get(part_fullname, cached, isolate_));
  if (found) {
    assert(!cached.is_null());
    return Handle<PyModule>::cast(cached);
  }

  // Slow Path: 走 module loader 加载模块
  Handle<PyList> search_path_list;
  ASSIGN_RETURN_ON_EXCEPTION(manager_->isolate(), search_path_list,
                             SelectSearchPathList(is_top, parent_module));

  return manager_->loader()->LoadModulePart(part_fullname, search_path_list);
}

MaybeHandle<PyList> ModuleImporter::SelectSearchPathList(
    bool is_top,
    Handle<PyObject> parent_module) {
  // 顶层模块直接走 sys.path 进行查找
  if (is_top) {
    assert(parent_module.is_null());
    return manager_->path();
  }

  Handle<PyList> path;
  RETURN_ON_EXCEPTION(
      isolate_, ModuleUtils::GetPackagePathList(isolate_, parent_module, path));

  if (path.is_null()) [[unlikely]] {
    Runtime_ThrowError(isolate_, ExceptionType::kImportError,
                       "parent is not a package");
    return kNullMaybe;
  }

  return path;
}

Maybe<void> ModuleImporter::BindChildModuleToParentNamespace(
    Handle<PyObject> parent_module,
    Handle<PyString> child_short_name,
    Handle<PyObject> child_module) {
  Handle<PyDict> parent_dict = PyObject::GetProperties(parent_module, isolate_);

  bool exists = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, exists,
      PyDict::ContainsKey(parent_dict, child_short_name, isolate_));

  if (!exists) {
    RETURN_ON_EXCEPTION(isolate_, PyDict::Put(parent_dict, child_short_name,
                                              child_module, isolate_));
  } else {
#ifdef _DEBUG
    // 如果父模块中已经链接了子模块，则不需要重复链接。
    // 但应确保该子模块与当前准备链接的child_module是同一个module object对象。
    Handle<PyObject> existing;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate_, found,
        parent_dict->Get(child_short_name, existing, isolate_));
    assert(found);
    assert(!existing.is_null());
    assert(existing.is_identical_to(child_module));
#endif
  }

  return JustVoid();
}

Maybe<void> ModuleImporter::EnsurePackageForNextSegment(
    Handle<PyObject> module,
    Handle<PyString> module_fullname) {
  bool is_package_module = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, is_package_module,
                             ModuleUtils::IsPackageModule(isolate_, module));

  if (!is_package_module) {
    Runtime_ThrowErrorf(isolate_, ExceptionType::kImportError,
                        "'%s' is not a package", module_fullname->buffer());
    return kNullMaybe;
  }

  return JustVoid();
}

MaybeHandle<PyModule> ModuleImporter::ApplyImportReturnSemantics(
    Handle<PyString> fullname,
    Handle<PyTuple> fromlist,
    Handle<PyModule> last_module) {
  bool has_fromlist = !fromlist.is_null() && fromlist->length() != 0;
  int64_t dot = fullname->IndexOf(ST(dot, isolate_));
  if (!has_fromlist && dot != PyString::kNotFound) {
    int64_t top_end = dot - 1;
    Handle<PyString> top_name = PyString::Slice(fullname, 0, top_end, isolate_);

    Handle<PyObject> top_module;
    bool found = false;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate_, found, modules_dict()->Get(top_name, top_module, isolate_));
    assert(found);
    assert(!top_module.is_null());

    return Handle<PyModule>::cast(top_module);
  }
  return last_module;
}

Handle<PyDict> ModuleImporter::modules_dict() {
  return manager_->modules();
}

}  // namespace saauso::internal
