// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-function.h"

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

// static
Handle<PyFunction> PyFunction::NewInstance(Handle<PyCodeObject> code_object) {
  HandleScope scope;
  Handle<PyFunction> object = NewInstanceInternal();

  object->func_code_ = *code_object;
  object->func_name_ = *code_object->co_name();
  object->flags_ = code_object->flags();

  // 绑定klass
  SetKlass(object, PyFunctionKlass::GetInstance());

  return object.EscapeFrom(&scope);
}

// static
Handle<PyFunction> PyFunction::NewInstance(NativeFuncPointer native_func,
                                           Handle<PyString> func_name) {
  HandleScope scope;
  Handle<PyFunction> object = NewInstanceInternal();

  object->native_func_ = native_func;
  object->func_name_ = *func_name;

  // 绑定klass
  SetKlass(object, NativeFunctionKlass::GetInstance());

  return object.EscapeFrom(&scope);
}

Handle<PyFunction> PyFunction::NewInstanceInternal() {
  Handle<PyFunction> object(Isolate::Current()->heap()->Allocate<PyFunction>(
      Heap::AllocationSpace::kNewSpace));

  // 初始化properties
  auto properties = PyDict::NewInstance();
  PyDict::Put(properties, PyString::NewInstance("__dict__"), properties);
  PyObject::SetProperties(*object, *properties);

  // 初始化字段
  object->func_code_ = Tagged<PyObject>::null();
  object->func_name_ = Tagged<PyObject>::null();
  object->func_globals_ = Tagged<PyObject>::null();
  object->default_args_ = Tagged<PyObject>::null();
  object->closures_ = Tagged<PyObject>::null();
  object->flags_ = 0;
  object->native_func_ = nullptr;

  return object;
}

// static
Tagged<PyFunction> PyFunction::cast(Tagged<PyObject> object) {
  assert(IsPyFunction(object));
  return Tagged<PyFunction>::cast(object);
}

///////////////////////////////////////////////////////////////////////////////

Handle<PyCodeObject> PyFunction::func_code() const {
  return handle(Tagged<PyCodeObject>::cast(func_code_));
}

Handle<PyString> PyFunction::func_name() const {
  return handle(Tagged<PyString>::cast(func_name_));
}

Handle<PyDict> PyFunction::func_globals() const {
  return handle(Tagged<PyDict>::cast(func_globals_));
}

void PyFunction::set_func_globals(Handle<PyDict> func_globals) {
  set_func_globals(*func_globals);
}

void PyFunction::set_func_globals(Tagged<PyDict> func_globals) {
  func_globals_ = func_globals;
}

Handle<PyTuple> PyFunction::default_args() const {
  return handle(Tagged<PyTuple>::cast(default_args_));
}

void PyFunction::set_default_args(Handle<PyTuple> default_args) {
  assert(default_args_.is_null());
  default_args_ = *default_args;
}

void PyFunction::set_closures(Handle<PyTuple> closures) {
  closures_ = *closures;
}

Handle<PyTuple> PyFunction::closures() const {
  return handle(closures_tagged());
}

Tagged<PyTuple> PyFunction::closures_tagged() const {
  return Tagged<PyTuple>::cast(closures_);
}

// static
Handle<MethodObject> MethodObject::NewInstance(Handle<PyObject> func,
                                               Handle<PyObject> owner) {
  Handle<MethodObject> object(
      Isolate::Current()->heap()->Allocate<MethodObject>(
          Heap::AllocationSpace::kNewSpace));
  object->owner_ = *owner;
  object->func_ = *func;
  PyObject::SetKlass(object, MethodObjectKlass::GetInstance());
  return object;
}

Tagged<MethodObject> MethodObject::cast(Tagged<PyObject> object) {
  assert(IsMethodObject(object));
  return Tagged<MethodObject>::cast(object);
}

}  // namespace saauso::internal
