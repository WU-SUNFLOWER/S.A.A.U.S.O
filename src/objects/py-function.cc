// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-function.h"

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"

namespace saauso::internal {

// static
Tagged<PyFunction> PyFunction::cast(Tagged<PyObject> object) {
  assert(IsPyFunction(object, Isolate::Current()));
  return Tagged<PyFunction>::cast(object);
}

///////////////////////////////////////////////////////////////////////////////

Handle<PyCodeObject> PyFunction::func_code(Isolate* isolate) const {
  return handle(Tagged<PyCodeObject>::cast(func_code_), isolate);
}

void PyFunction::set_func_code(Handle<PyCodeObject> code) {
  set_func_code(*code);
}

void PyFunction::set_func_code(Tagged<PyCodeObject> code) {
  func_code_ = code;
  WRITE_BARRIER(Tagged<PyObject>(this), &func_code_, code);
}

Handle<PyString> PyFunction::func_name(Isolate* isolate) const {
  return handle(Tagged<PyString>::cast(func_name_), isolate);
}

void PyFunction::set_func_name(Handle<PyString> name) {
  set_func_name(*name);
}

void PyFunction::set_func_name(Tagged<PyString> name) {
  func_name_ = name;
  WRITE_BARRIER(Tagged<PyObject>(this), &func_name_, name);
}

Handle<PyDict> PyFunction::func_globals(Isolate* isolate) const {
  return handle(Tagged<PyDict>::cast(func_globals_), isolate);
}

void PyFunction::set_func_globals(Handle<PyDict> func_globals) {
  set_func_globals(*func_globals);
}

void PyFunction::set_func_globals(Tagged<PyDict> func_globals) {
  func_globals_ = func_globals;
  WRITE_BARRIER(Tagged<PyObject>(this), &func_globals_,
                func_globals);
}

Handle<PyTuple> PyFunction::default_args(Isolate* isolate) const {
  return handle(Tagged<PyTuple>::cast(default_args_), isolate);
}

void PyFunction::set_default_args(Handle<PyTuple> default_args) {
  set_default_args(*default_args);
}

void PyFunction::set_default_args(Tagged<PyTuple> default_args) {
  default_args_ = default_args;
  WRITE_BARRIER(Tagged<PyObject>(this), &default_args_,
                default_args);
}

void PyFunction::set_closures(Handle<PyTuple> closures) {
  set_closures(*closures);
}

void PyFunction::set_closures(Tagged<PyTuple> closures) {
  closures_ = closures;
  WRITE_BARRIER(Tagged<PyObject>(this), &closures_, closures);
}

Handle<PyTuple> PyFunction::closures(Isolate* isolate) const {
  return handle(closures_tagged(), isolate);
}

Tagged<PyTuple> PyFunction::closures_tagged() const {
  return Tagged<PyTuple>::cast(closures_);
}

void PyFunction::set_native_closure_data(Handle<PyObject> closure_data) {
  set_native_closure_data(*closure_data);
}

void PyFunction::set_native_closure_data(Tagged<PyObject> closure_data) {
  native_closure_data_ = closure_data;
  WRITE_BARRIER(Tagged<PyObject>(this), &native_closure_data_, closure_data);
}

void PyFunction::set_native_owner_type(Handle<PyTypeObject> owner_type) {
  set_native_owner_type(*owner_type);
}

void PyFunction::set_native_owner_type(Tagged<PyTypeObject> owner_type) {
  native_owner_type_ = owner_type;
  WRITE_BARRIER(Tagged<PyObject>(this), &native_owner_type_,
                owner_type);
}

void MethodObject::set_owner(Handle<PyObject> owner) {
  set_owner(*owner);
}

void MethodObject::set_owner(Tagged<PyObject> owner) {
  owner_ = owner;
  WRITE_BARRIER(Tagged<PyObject>(this), &owner_, owner);
}

void MethodObject::set_func(Handle<PyObject> func) {
  set_func(*func);
}

void MethodObject::set_func(Tagged<PyObject> func) {
  func_ = func;
  WRITE_BARRIER(Tagged<PyObject>(this), &func_, func);
}

Tagged<MethodObject> MethodObject::cast(Tagged<PyObject> object) {
  assert(IsMethodObject(object));
  return Tagged<MethodObject>::cast(object);
}

}  // namespace saauso::internal
