// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/accessor-proxy.h"

#include "src/builtins/accessors.h"
#include "src/execution/exception-utils.h"
#include "src/objects/py-string.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Maybe<bool> AccessorProxy::TryGet(Isolate* isolate,
                                  Handle<PyObject> receiver,
                                  Handle<PyObject> name,
                                  Handle<PyObject>& out_value) {
  out_value = Handle<PyObject>::null();

  const AccessorDescriptor* descriptor = LookupAccessor(name);
  if (descriptor == nullptr || descriptor->getter == nullptr) {
    return Maybe<bool>(false);
  }

  Handle<PyObject> value;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, value,
                             descriptor->getter(isolate, receiver));

  out_value = value;
  return Maybe<bool>(true);
}

Maybe<bool> AccessorProxy::TrySet(Isolate* isolate,
                                  Handle<PyObject> receiver,
                                  Handle<PyObject> name,
                                  Handle<PyObject> value) {
  const AccessorDescriptor* descriptor = LookupAccessor(name);
  if (descriptor == nullptr || descriptor->setter == nullptr) {
    return Maybe<bool>(false);
  }

  RETURN_ON_EXCEPTION(isolate, descriptor->setter(isolate, receiver, value));

  return Maybe<bool>(true);
}

const AccessorDescriptor* AccessorProxy::LookupAccessor(Handle<PyObject> name) {
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
