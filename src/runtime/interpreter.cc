// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/runtime/interpreter.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/native-functions.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

Interpreter::Interpreter() {
  HandleScope scope;

  Handle<PyDict> builtins = PyDict::NewInstance();

  // 注册基础类型的klass
  PyDict::Put(builtins, PyString::NewInstance("object"),
              PyObjectKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("int"),
              PySmiKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("str"),
              PyStringKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("list"),
              PyListKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("bool"),
              PyBooleanKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("dict"),
              PyDictKlass::GetInstance()->type_object());

  // 注册oddballs
  PyDict::Put(builtins, PyString::NewInstance("True"),
              handle(Universe::py_true_object_));
  PyDict::Put(builtins, PyString::NewInstance("False"),
              handle(Universe::py_false_object_));
  PyDict::Put(builtins, PyString::NewInstance("None"),
              handle(Universe::py_none_object_));

  // 注册native function
  auto func_name = PyString::NewInstance("print");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_Print, func_name));
  func_name = PyString::NewInstance("len");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_Len, func_name));

  func_name = PyString::NewInstance("isinstance");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_IsInstance, func_name));

  builtins_ = *builtins;
}

Handle<PyObject> Interpreter::builtins() const {
  return handle(builtins_);
}

Handle<PyObject> Interpreter::CallVirtual(Handle<PyObject> callable,
                                          Handle<PyList> args) {
  HandleScope scope;
  Handle<PyList> actual_args = args.IsNull() ? PyList::NewInstance() : args;

  if (IsPyNativeFunction(callable)) {
    return PyObject::Call(callable, args, Handle<PyObject>::Null());
  }

  if (IsPyFunction(callable)) {
    // TODO: 创建新的虚拟机栈帧，执行python代码
    return handle(Universe::py_none_object_);
  }

  if (IsMethodObject(callable)) {
    auto method = Handle<MethodObject>::cast(callable);
    PyList::Insert(actual_args, 0, method->owner());
    return CallVirtual(method->func(), actual_args);
  }

  assert(0 && "unreachable");
  return Handle<PyObject>::Null();
}

void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&builtins_);
}

}  // namespace saauso::internal
