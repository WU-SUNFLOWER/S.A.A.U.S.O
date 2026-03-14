// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/accessor-manager.h"

#include "src/builtins/accessors.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Maybe<bool> AccessorManager::TryGet(Handle<PyObject> receiver,
                                    Handle<PyObject> name,
                                    Handle<PyObject>& out_value) {
  const AccessorDescriptor* descriptor = LookupAccessor(name);
  if (descriptor == nullptr || descriptor->getter == nullptr) {
    return Maybe<bool>(false);
  }

  Handle<PyObject> value;
  if (!descriptor->getter(receiver).ToHandle(&value)) {
    out_value = Handle<PyObject>::null();
    return kNullMaybe;
  }

  out_value = value;
  return Maybe<bool>(true);
}

Maybe<bool> AccessorManager::TrySet(Handle<PyObject> receiver,
                                    Handle<PyObject> name,
                                    Handle<PyObject> value) {
  const AccessorDescriptor* descriptor = LookupAccessor(name);
  if (descriptor == nullptr || descriptor->setter == nullptr) {
    return Maybe<bool>(false);
  }

  if (descriptor->setter(receiver, value).IsEmpty()) {
    return kNullMaybe;
  }

  return Maybe<bool>(true);
}

const AccessorDescriptor* AccessorManager::LookupAccessor(
    Handle<PyObject> name) {
  if (!IsPyString(name)) {
    return nullptr;
  }
  Handle<PyString> str_name = Handle<PyString>::cast(name);
  if (str_name->IsEqualTo(ST_TAGGED(class))) {
    return &Accessors::kPyObjectClassAccessor;
  }
  if (str_name->IsEqualTo(ST_TAGGED(dict))) {
    return &Accessors::kPyObjectDictAccessor;
  }
  return nullptr;
}

}  // namespace saauso::internal
