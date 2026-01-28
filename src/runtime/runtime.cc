// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/runtime.h"

#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/string-table.h"

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
  if (IsPyString(object)) {
    return Tagged<PyString>::cast(object)->length() != 0;
  }
  if (IsPyList(object)) {
    return Tagged<PyList>::cast(object)->length() != 0;
  }
  if (IsPyTuple(object)) {
    return Tagged<PyTuple>::cast(object)->length() != 0;
  }
  if (IsPyDict(object)) {
    return Tagged<PyDict>::cast(object)->occupied() != 0;
  }
  return true;
}

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable) {
  HandleScope scope;

  auto source_iterator = PyObject::Iter(iteratable);
  auto iterator_next_func = PyObject::GetAttr(source_iterator, ST(next));

#define CALL_NEXT_FUNC()                         \
  Isolate::Current()->interpreter()->CallPython( \
      iterator_next_func, Handle<PyTuple>::Null(), Handle<PyDict>::Null())

  auto elem = CALL_NEXT_FUNC();
  while (!elem.IsNull()) {
    PyList::Append(list, elem);
    elem = CALL_NEXT_FUNC();
  }

#undef CALL_NEXT_FUNC
}

Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable) {
  HandleScope scope;
  Handle<PyList> tmp = PyList::NewInstance();
  Runtime_ExtendListByItratableObject(tmp, iterable);
  Handle<PyTuple> tuple = PyTuple::NewInstance(tmp->length());
  for (int64_t i = 0; i < tmp->length(); ++i) {
    tuple->SetInternal(i, tmp->Get(i));
  }
  return tuple;
}

}  // namespace saauso::internal
