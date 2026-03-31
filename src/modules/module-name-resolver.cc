// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-name-resolver.h"

#include <cassert>

#include "src/execution/exception-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

MaybeHandle<PyString> ModuleNameResolver::ResolveFullName(
    Handle<PyString> name,
    int64_t level,
    Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate_);
  assert(level >= 0);

  // 只有在相对导入的情况下，才允许模块 name 为空，例如 `from .. import
  // my_module`
  if (level == 0 && (name.is_null() || name->length() == 0)) {
    Runtime_ThrowError(isolate_, ExceptionType::kModuleNotFoundError,
                       "empty module name");
    return kNullMaybe;
  }

  Handle<PyString> fullname;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, fullname,
                             ResolveRelativeImportName(name, level, globals));

  return scope.Escape(fullname);
}

MaybeHandle<PyString> ModuleNameResolver::ResolveRelativeImportName(
    Handle<PyString> name,
    int64_t level,
    Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate_);

  // 如果不是相对导入，直接返回即可
  if (level <= 0) {
    if (name.is_null()) {
      return scope.Escape(PyString::New(isolate_, ""));
    }
    return scope.Escape(name);
  }

  Handle<PyString> base;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, base,
                             ResolvePackageFromGlobals(globals));

  if (base->length() == 0) {
    Runtime_ThrowError(
        isolate_, ExceptionType::kImportError,
        "attempted relative import with no known parent package");
    return kNullMaybe;
  }

  for (int i = 1; i < level; ++i) {
    Handle<PyString> parent = ParentModuleNameOrEmpty(base);
    if (parent->length() == 0) {
      Runtime_ThrowError(isolate_, ExceptionType::kImportError,
                         "attempted relative import beyond top-level package");
      return kNullMaybe;
    }
    base = parent;
  }

  if (name.is_null() || name->length() == 0) {
    return scope.Escape(base);
  }

  Handle<PyString> fullname = PyString::Append(base, ST(dot, isolate_), isolate_);
  fullname = PyString::Append(fullname, name, isolate_);
  return scope.Escape(fullname);
}

MaybeHandle<PyString> ModuleNameResolver::ResolvePackageFromGlobals(
    Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate_);

  if (globals.is_null()) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  Handle<PyObject> package_obj;
  bool found = false;
  if (!globals->Get(ST(package, isolate_), package_obj, isolate_).To(&found)) {
    return kNullMaybe;
  }
  if (found) {
    if (!IsPyString(package_obj)) {
      Runtime_ThrowError(isolate_, ExceptionType::kTypeError,
                         "__package__ must be a string");
      return kNullMaybe;
    }
    return scope.Escape(Handle<PyString>::cast(package_obj));
  }

  Tagged<PyObject> name_obj;
  if (!globals->GetTagged(ST(name, isolate_), name_obj, isolate_).To(&found)) {
    return kNullMaybe;
  }
  if (!found || !IsPyString(name_obj)) {
    Runtime_ThrowError(isolate_, ExceptionType::kImportError,
                       "missing __name__ in globals");
    return kNullMaybe;
  }

  Handle<PyString> name_str =
      Handle<PyString>::cast(handle(name_obj, isolate_));

  Tagged<PyObject> path_obj;
  if (!globals->GetTagged(ST(path, isolate_), path_obj, isolate_).To(&found)) {
    return kNullMaybe;
  }
  bool is_package = found;
  if (is_package) {
    return scope.Escape(name_str);
  }
  return scope.Escape(ParentModuleNameOrEmpty(name_str));
}

Handle<PyString> ModuleNameResolver::ParentModuleNameOrEmpty(
    Handle<PyString> name) {
  EscapableHandleScope scope(isolate_);

  if (name.is_null() || name->length() == 0) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  auto dot_index = name->LastIndexOf(ST(dot, isolate_));
  if (dot_index == PyString::kNotFound || dot_index == 0) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  int64_t parent_end = dot_index - 1;
  return scope.Escape(PyString::Slice(name, 0, parent_end, isolate_));
}

}  // namespace saauso::internal
