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

  // 只有在相对导入的情况下，才允许模块 name 为空。
  // 例如：
  // - from .. import helper
  //   CPython 字节码会把 name 传成空串/null，再由 level=2 与当前包名推导出
  //   "pkg"
  // - import helper
  //   则必须显式给出非空 name
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

  // 绝对导入不需要结合当前模块上下文，直接使用传入 name。
  // 例如：
  // - import pkg.sub => 返回 "pkg.sub"
  // - from pkg import sub => 这里的 IMPORT_NAME 目标仍是 "pkg"
  if (level <= 0) {
    if (name.is_null()) {
      return scope.Escape(PyString::New(isolate_, ""));
    }
    return scope.Escape(name);
  }

  Handle<PyString> base;
  ASSIGN_RETURN_ON_EXCEPTION(isolate_, base,
                             ResolvePackageFromGlobals(globals));

  // base 表示“当前模块所属的包”。
  // 例如当前执行的是 pkg.tools.impl.runner：
  // - from . import util      => 初始 base = "pkg.tools.impl"
  // - from ..core import api  => 初始 base = "pkg.tools.impl"
  if (base->length() == 0) {
    Runtime_ThrowError(
        isolate_, ExceptionType::kImportError,
        "attempted relative import with no known parent package");
    return kNullMaybe;
  }

  // level=1 表示停留在当前包；
  // level=2 表示向上取一层父包；
  // level=3 表示再向上一层，以此类推。
  // 例如 base = "pkg.tools.impl"：
  // - level=1 => "pkg.tools.impl"
  // - level=2 => "pkg.tools"
  // - level=3 => "pkg"
  for (int i = 1; i < level; ++i) {
    Handle<PyString> parent = ParentModuleNameOrEmpty(base);
    if (parent->length() == 0) {
      Runtime_ThrowError(isolate_, ExceptionType::kImportError,
                         "attempted relative import beyond top-level package");
      return kNullMaybe;
    }
    base = parent;
  }

  // name 为空时，说明 import 目标就是“上面求得的包本身”。
  // 例如 from .. import helper 中，IMPORT_NAME 对应的 fullname 是 "pkg"，
  // 后续再由 IMPORT_FROM 从该包对象上取出属性 helper。
  if (name.is_null() || name->length() == 0) {
    return scope.Escape(base);
  }

  // 普通相对导入则把“基包 + '.' + name”拼起来。
  // 例如：
  // - base="pkg.tools", name="util" => "pkg.tools.util"
  // - base="pkg", name="core" => "pkg.core"
  Handle<PyString> fullname =
      PyString::Append(base, ST(dot, isolate_), isolate_);
  fullname = PyString::Append(fullname, name, isolate_);
  return scope.Escape(fullname);
}

MaybeHandle<PyString> ModuleNameResolver::ResolvePackageFromGlobals(
    Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate_);

  // 没有 globals 时，无法知道当前模块属于哪个包。
  // 这意味着后续若请求相对导入，会被上层逻辑判定为“没有已知父包”。
  if (globals.is_null()) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  Handle<PyObject> package_obj;
  bool found = false;
  if (!PyDict::Get(globals, ST(package, isolate_), package_obj, isolate_)
           .To(&found)) {
    return kNullMaybe;
  }
  if (found) {
    // __package__ 是最直接、最可靠的包上下文。
    // 例如：
    // - __package__ = "pkg.tools"
    //   则 from . import util 会以 "pkg.tools" 为基准继续解析
    if (!IsPyString(package_obj)) {
      Runtime_ThrowError(isolate_, ExceptionType::kTypeError,
                         "__package__ must be a string");
      return kNullMaybe;
    }
    return scope.Escape(Handle<PyString>::cast(package_obj));
  }

  Handle<PyObject> name_obj;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, found,
      PyDict::Get(globals, ST(name, isolate_), name_obj, isolate_));

  if (!found || !IsPyString(name_obj)) {
    Runtime_ThrowError(isolate_, ExceptionType::kImportError,
                       "missing __name__ in globals");
    return kNullMaybe;
  }

  Handle<PyString> name_str = Handle<PyString>::cast(name_obj);

  Handle<PyObject> path_obj;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, found,
      PyDict::Get(globals, ST(path, isolate_), path_obj, isolate_));

  // __path__ 存在表示当前模块对象本身就是“包”；
  // 否则把 __name__ 当作普通模块名，并退回到它的父包。
  // 例如：
  // - __name__="pkg.tools",   有 __path__ => 当前就是包，返回 "pkg.tools"
  // - __name__="pkg.tools.x", 无 __path__ => 当前是模块，返回 "pkg.tools"
  bool is_package = found;
  if (is_package) {
    return scope.Escape(name_str);
  }
  return scope.Escape(ParentModuleNameOrEmpty(name_str));
}

Handle<PyString> ModuleNameResolver::ParentModuleNameOrEmpty(
    Handle<PyString> name) {
  EscapableHandleScope scope(isolate_);

  // 空串、顶层模块名、或不含 '.' 的名字，都没有父模块。
  if (name.is_null() || name->length() == 0) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  // 例子：
  // - "pkg.tools.impl" => dot_index 指向最后一个 '.'，父名是 "pkg.tools"
  // - "pkg" => 没有 '.'，返回空串
  auto dot_index = name->LastIndexOf(ST(dot, isolate_));
  if (dot_index == PyString::kNotFound || dot_index == 0) {
    return scope.Escape(PyString::New(isolate_, ""));
  }

  int64_t parent_end = dot_index - 1;
  return scope.Escape(PyString::Slice(name, 0, parent_end, isolate_));
}

}  // namespace saauso::internal
