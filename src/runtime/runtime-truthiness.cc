// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime-truthiness.h"

#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

bool Runtime_PyObjectIsTrue(Handle<PyObject> object) {
  return Runtime_PyObjectIsTrue(*object);
}

bool Runtime_PyObjectIsTrue(Tagged<PyObject> object) {
  if (IsPyFalse(object) || IsPyNone(object)) {
    return false;
  }
  if (IsPyTrue(object)) {
    return true;
  }
  if (IsPySmi(object)) {
    return Tagged<PySmi>::cast(object).value() != 0;
  }
  if (IsPyFloat(object)) {
    return Tagged<PyFloat>::cast(object)->value() != 0.0;
  }
  if (PyString::IsStringLike(object)) {
    return Tagged<PyString>::cast(object)->length() != 0;
  }
  if (PyList::IsListLike(object)) {
    return Tagged<PyList>::cast(object)->length() != 0;
  }
  if (PyTuple::IsTupleLike(object)) {
    return Tagged<PyTuple>::cast(object)->length() != 0;
  }
  if (IsPyDict(object)) {
    return Tagged<PyDict>::cast(object)->occupied() != 0;
  }
  return true;
}

bool Runtime_IsPyObjectCallable(Tagged<PyObject> object) {
  if (object.is_null()) {
    return false;
  }

  if (IsPyFunction(object) || IsMethodObject(object)) {
    return true;
  }

  Tagged<Klass> klass = PyObject::GetKlass(object);
  if (klass->vtable().call != nullptr) {
    return true;
  }

  return false;
}

}  // namespace saauso::internal
