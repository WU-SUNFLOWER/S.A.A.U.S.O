// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/native-functions.h"

#include "src/handles/tagged.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

Handle<PyObject> Native_Print(Handle<PyList> args, Handle<PyDict> kwargs) {
  for (auto i = 0; i < args->length(); ++i) {
    HandleScope scope;
    PyObject::Print(args->Get(i));
    std::printf(" ");
  }
  std::printf("\n");
  return handle(Universe::py_none_object_);
}

Handle<PyObject> Native_Len(Handle<PyList> args, Handle<PyDict> kwargs) {
  return PyObject::Len(args->Get(0));
}

}  // namespace saauso::internal
