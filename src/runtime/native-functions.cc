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

Handle<PyObject> Native_IsInstance(Handle<PyList> args, Handle<PyDict> kwargs) {
  HandleScope scope;

  auto object = args->Get(0);
  auto mro_of_object = PyObject::GetKlass(object)->mro();
  auto target_type_object = args->Get(1);

  for (auto i = 0; i < mro_of_object->length(); ++i) {
    auto curr_type_object = mro_of_object->Get(i);
    if (IsPyTrue(PyObject::Equal(curr_type_object, target_type_object))) {
      return handle(Universe::py_true_object_);
    }
  }

  return handle(Universe::py_false_object_);
}

}  // namespace saauso::internal
