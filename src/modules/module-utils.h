// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_MODULES_MODULE_UTILS_H_
#define SAAUSO_MODULES_MODULE_UTILS_H_

#include <string>
#include <string_view>

#include "src/handles/handles.h"

namespace saauso::internal {

class PyDict;
class PyList;
class PyObject;
class PyString;

class ModuleUtils final {
 public:
  static Handle<PyString> NewPyString(std::string_view s);

  static bool IsValidModuleName(Handle<PyString> fullname);

  static bool IsPackageModule(Handle<PyObject> module);
  static Handle<PyList> GetPackagePathList(Handle<PyObject> module);
};

}  // namespace saauso::internal

#endif  // SAAUSO_MODULES_MODULE_UTILS_H_
