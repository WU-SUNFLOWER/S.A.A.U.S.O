// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-tuple.h"

#include <string>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

MaybeHandle<PyString> Runtime_NewTupleRepr(Handle<PyTuple> tuple) {
  EscapableHandleScope scope;
  auto* isolate = Isolate::Current();

  std::string repr("(");
  for (int64_t i = 0; i < tuple->length(); ++i) {
    if (i > 0) {
      repr.append(", ");
    }
    Handle<PyString> elem;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, elem, PyObject::Repr(tuple->Get(i)));
    repr.append(elem->ToStdString());
  }
  if (tuple->length() == 1) {
    repr.push_back(',');
  }
  repr.push_back(')');

  return scope.Escape(PyString::FromStdString(repr));
}

}  // namespace saauso::internal
