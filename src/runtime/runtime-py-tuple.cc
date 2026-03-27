// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-py-tuple.h"

#include <algorithm>
#include <string>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

MaybeHandle<PyString> Runtime_NewTupleRepr(Isolate* isolate,
                                           Handle<PyTuple> tuple) {
  EscapableHandleScope scope;

  std::string repr("(");
  for (int64_t i = 0; i < tuple->length(); ++i) {
    if (i > 0) {
      repr.append(", ");
    }
    Handle<PyString> elem;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, elem,
                               PyObject::Repr(isolate, tuple->Get(i)));
    repr.append(elem->ToStdString());
  }
  if (tuple->length() == 1) {
    repr.push_back(',');
  }
  repr.push_back(')');

  return scope.Escape(PyString::FromStdString(repr));
}

Handle<PyTuple> Runtime_NewTupleSlice(Isolate* isolate,
                                      Handle<PyTuple> tuple,
                                      int64_t begin,
                                      int64_t end) {
  if (tuple.is_null()) {
    return Handle<PyTuple>::null();
  }

  int64_t length = tuple->length();
  begin = std::max(static_cast<int64_t>(0), std::min(begin, length));
  end = std::max(begin, std::min(end, length));

  Handle<PyTuple> sliced = PyTuple::New(isolate, end - begin);
  for (int64_t i = begin; i < end; ++i) {
    sliced->SetInternal(i - begin, tuple->GetTagged(i));
  }
  return sliced;
}

Handle<PyTuple> Runtime_NewTupleTailOrNull(Isolate* isolate,
                                           Handle<PyTuple> tuple,
                                           int64_t begin) {
  if (tuple.is_null()) {
    return Handle<PyTuple>::null();
  }
  if (begin >= tuple->length()) {
    return Handle<PyTuple>::null();
  }
  return Runtime_NewTupleSlice(isolate, tuple, begin, tuple->length());
}

}  // namespace saauso::internal
