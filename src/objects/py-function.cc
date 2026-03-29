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

Handle<PyCodeObject> PyFunction::func_code() const {
  return handle(Tagged<PyCodeObject>::cast(func_code_));
}

Handle<PyString> PyFunction::func_name() const {
  return handle(Tagged<PyString>::cast(func_name_));
}

void PyFunction::set_func_name(Handle<PyString> name) {
  func_name_ = *name;
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

Tagged<MethodObject> MethodObject::cast(Tagged<PyObject> object) {
  assert(IsMethodObject(object));
  return Tagged<MethodObject>::cast(object);
}

}  // namespace saauso::internal
