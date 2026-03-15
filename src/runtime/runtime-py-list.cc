// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-list.h"

#include <string>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

MaybeHandle<PyString> Runtime_NewListRepr(Handle<PyList> list) {
  EscapableHandleScope scope;
  auto* isolate = Isolate::Current();

  std::string repr("[");
  for (int64_t i = 0; i < list->length(); ++i) {
    if (i > 0) {
      repr.append(", ");
    }
    Handle<PyString> elem;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, elem, PyObject::Repr(list->Get(i)));
    repr.append(elem->ToStdString());
  }
  repr.append("]");

  return scope.Escape(PyString::FromStdString(repr));
}

}  // namespace saauso::internal
