// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/interpreter.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

Interpreter::Interpreter() {
  HandleScope scope;

  Handle<PyDict> builtins = PyDict::NewInstance();
  PyDict::Put(builtins, PyString::NewInstance("int"),
              PySmiKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("str"),
              PyStringKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("list"),
              PyListKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("dict"),
              PyDictKlass::GetInstance()->type_object());
}

Handle<PyObject> Interpreter::CallVirtual(Handle<PyObject> func,
                                          Handle<PyList> args) {
  // TODO: 实现call virtual
  return Handle<PyObject>::Null();
}

void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&builtins_);
}

}  // namespace saauso::internal
