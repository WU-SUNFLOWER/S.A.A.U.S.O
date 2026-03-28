// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/modules/module-utils.h"

#include "src/execution/exception-utils.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Handle<PyString> ModuleUtils::NewPyString(std::string_view s) {
  if (s.empty()) {
    return PyString::NewInstance("");
  }
  return PyString::NewInstance(s.data(), static_cast<int64_t>(s.size()));
}

bool ModuleUtils::IsValidModuleName(Handle<PyString> fullname) {
  if (fullname->IsEmpty()) {
    return false;
  }
  if (fullname->front() == '.' || fullname->back() == '.') {
    return false;
  }
  for (int64_t i = 1; i < fullname->length(); ++i) {
    if (fullname->Get(i) == '.' && fullname->Get(i - 1) == '.') {
      return false;
    }
  }
  return true;
}

Maybe<bool> ModuleUtils::IsPackageModule(Isolate* isolate,
                                         Handle<PyObject> module) {
  Handle<PyDict> dict = PyObject::GetProperties(module);
  if (dict.is_null()) {
    return Maybe<bool>(false);
  }

  Tagged<PyObject> path;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             dict->GetTagged(ST(path), path, isolate));

  return Maybe<bool>(found && IsPyList(path));
}

Maybe<void> ModuleUtils::GetPackagePathList(Isolate* isolate,
                                            Handle<PyObject> module,
                                            Handle<PyList>& out) {
  out = Handle<PyList>::null();

  Handle<PyDict> dict = PyObject::GetProperties(module);
  if (dict.is_null()) {
    return JustVoid();
  }

  Tagged<PyObject> path_obj;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             dict->GetTagged(ST(path), path_obj, isolate));

  if (!found || !IsPyList(path_obj)) {
    return JustVoid();
  }

  out = Handle<PyList>::cast(handle(path_obj));
  return JustVoid();
}

}  // namespace saauso::internal
