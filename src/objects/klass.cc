// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"

#include "src/handles/tagged.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/interpreter.h"
#include "src/runtime/string-table.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

namespace {

Handle<PyObject> FindAndCall(Handle<PyObject> object,
                             Handle<PyList> args,
                             Handle<PyObject> func_name) {
  Handle<PyObject> func = PyObject::GetAttr(object, func_name);
  if (!IsPyNone(func)) {
    return Universe::interpreter_->CallVirtual(func, args);
  }

  std::printf("class ");
  PyObject::Print(PyObject::GetKlass(object)->name());
  std::printf(" Error : unsupport operation for class ");
  std::exit(1);

  return handle(Universe::py_none_object_);
}

}  // namespace

///////////////////////////////////////////////////////////////////////

VirtualTable::VirtualTable()
    : add(&Klass::Virtual_Default_Add),
      getattr(&Klass::Virtual_Default_GetAttr),
      setattr(&Klass::Virtual_Default_SetAttr),
      subscr(&Klass::Virtual_Default_Subscr),
      store_subscr(&Klass::Virtual_Default_StoreSubscr),
      next(&Klass::Virtual_Default_Next),
      call(&Klass::Virtual_Default_Call),
      len(&Klass::Virtual_Default_Len),
      print(&Klass::Virtual_Default_Print),
      repr(&Klass::Virtual_Default_Repr),
      del_subscr(&Klass::Virtual_Default_Delete_Subscr),
      instance_size(&Klass::Virtual_Default_InstanceSize) {}

///////////////////////////////////////////////////////////////////////

Handle<PyString> Klass::name() {
  return Handle<PyString>(Tagged<PyString>::cast(name_));
}

void Klass::set_name(Handle<PyString> name) {
  name_ = *name;
}

Handle<PyTypeObject> Klass::type_object() {
  return Handle<PyTypeObject>(Tagged<PyTypeObject>::cast(name_));
}

void Klass::set_type_object(Handle<PyTypeObject> type_object) {
  type_object_ = *type_object;
}

Handle<PyDict> Klass::klass_properties() {
  return handle(Tagged<PyDict>::cast(klass_properties_));
}

void Klass::set_klass_properties(Handle<PyDict> klass_properties) {
  type_object_ = *klass_properties;
}

void Klass::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&name_);
  v->VisitPointer(&type_object_);
  v->VisitPointer(&klass_properties_);
}

///////////////////////////////////////////////////////////////////////

void Klass::Virtual_Default_Print(Handle<PyObject> self) {
  Handle<PyObject> func = PyObject::GetAttr(self, ST(str));
  if (IsPyNone(func)) {
    func = PyObject::GetAttr(self, ST(repr));
  }

  if (!IsPyNone(func)) {
    Handle<PyObject> s =
        Universe::interpreter_->CallVirtual(func, Handle<PyList>::Null());
    PyObject::Print(Handle<PyString>::cast(s));
    return;
  }

  std::printf("<object at %p>", reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> Klass::Virtual_Default_Len(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(len));
}

Handle<PyObject> Klass::Virtual_Default_Repr(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(repr));
}

Handle<PyObject> Klass::Virtual_Default_Call(Handle<PyObject> self,
                                             Handle<PyObject> args,
                                             Handle<PyObject> kwargs) {
  Handle<PyObject> callable = PyObject::GetAttr(self, ST(call));
  if (IsPyNone(callable)) {
    PyObject::Print(callable);
    std::printf(" is non-callable\n");
    std::exit(1);
  }

  return Universe::interpreter_->CallVirtual(callable,
                                             Handle<PyList>::cast(args));
}

Handle<PyObject> Klass::Virtual_Default_GetAttr(
    Handle<PyObject> self,
    Handle<PyObject> property_name) {
  // TODO: 实现getattr逻辑
}

void Klass::Virtual_Default_SetAttr(Handle<PyObject> self,
                                    Handle<PyObject> property_name,
                                    Handle<PyObject> property_value) {
  auto properties = PyObject::GetProperties(self);
  PyDict::Put(properties, property_name, property_value);
}

Handle<PyObject> Klass::Virtual_Default_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  return FindAndCall(self, args, ST(getitem));
}

void Klass::Virtual_Default_StoreSubscr(Handle<PyObject> self,
                                        Handle<PyObject> subscr,
                                        Handle<PyObject> value) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  PyList::Append(args, value);
  FindAndCall(self, args, ST(setitem));
}

void Klass::Virtual_Default_Delete_Subscr(Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, subscr);
  FindAndCall(self, args, ST(delitem));
}

Handle<PyObject> Klass::Virtual_Default_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  Handle<PyList> args = PyList::NewInstance();
  PyList::Append(args, other);
  return FindAndCall(self, args, ST(add));
}

Handle<PyObject> Klass::Virtual_Default_Next(Handle<PyObject> self) {
  return FindAndCall(self, Handle<PyList>::Null(), ST(getitem));
}

size_t Klass::Virtual_Default_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyObject);
}

}  // namespace saauso::internal
