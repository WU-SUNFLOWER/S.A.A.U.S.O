// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-utils.h"

#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

std::string ModuleUtils::ToStdString(Handle<PyString> s) {
  if (s.is_null()) {
    return std::string();
  }
  return std::string(s->buffer(), static_cast<size_t>(s->length()));
}

std::vector<std::string> ModuleUtils::SplitModuleName(std::string_view name) {
  std::vector<std::string> parts;
  size_t start = 0;
  while (start <= name.size()) {
    size_t dot = name.find('.', start);
    if (dot == std::string_view::npos) {
      dot = name.size();
    }
    if (dot == start) {
      return {};
    }
    parts.emplace_back(name.substr(start, dot - start));
    if (dot == name.size()) {
      break;
    }
    start = dot + 1;
  }
  return parts;
}

std::string ModuleUtils::JoinModuleName(const std::vector<std::string>& parts,
                                        size_t count) {
  std::string out;
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      out.push_back('.');
    }
    out.append(parts[i]);
  }
  return out;
}

bool ModuleUtils::IsPackageModule(Handle<PyObject> module) {
  Handle<PyDict> dict = PyObject::GetProperties(module);
  if (dict.is_null()) {
    return false;
  }
  Handle<PyObject> path = dict->Get(PyString::NewInstance("__path__"));
  return !path.is_null() && IsPyList(path);
}

Handle<PyList> ModuleUtils::GetPackagePathList(Handle<PyObject> module) {
  Handle<PyDict> dict = PyObject::GetProperties(module);
  if (dict.is_null()) {
    return Handle<PyList>::null();
  }
  Handle<PyObject> path_obj = dict->Get(PyString::NewInstance("__path__"));
  if (path_obj.is_null() || !IsPyList(path_obj)) {
    return Handle<PyList>::null();
  }
  return Handle<PyList>::cast(path_obj);
}

}  // namespace saauso::internal

