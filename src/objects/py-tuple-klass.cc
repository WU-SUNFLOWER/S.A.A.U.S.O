// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-klass.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PyTupleKlass> PyTupleKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyTupleKlass> instance = isolate->py_tuple_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTupleKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_tuple_klass(instance);
  }
  return instance;
}

void PyTupleKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(this);

  vtable_.len = &Virtual_Len;
  vtable_.print = &Virtual_Print;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DelSubscr;
  vtable_.contains = &Virtual_Contains;
  vtable_.equal = &Virtual_Equal;
  vtable_.iter = &Virtual_Iter;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyTupleKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("tuple"));
}

void PyTupleKlass::Finalize() {
  Isolate::Current()->set_py_tuple_klass(Tagged<PyTupleKlass>::null());
}

Handle<PyObject> PyTupleKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyTuple>::cast(self)->length()));
}

void PyTupleKlass::Virtual_Print(Handle<PyObject> self) {
  auto tuple = Handle<PyTuple>::cast(self);

  std::printf("(");

  if (tuple->length() > 0) {
    PyObject::Print(tuple->Get(0));
  }

  for (auto i = 1; i < tuple->length(); ++i) {
    std::printf(", ");
    PyObject::Print(tuple->Get(i));
  }

  if (tuple->length() == 1) {
    std::printf(",");
  }

  std::printf(")");
}

Handle<PyObject> PyTupleKlass::Virtual_Subscr(Handle<PyObject> self,
                                              Handle<PyObject> subscr) {
  auto tuple = Handle<PyTuple>::cast(self);
  if (!IsPySmi(*subscr)) {
    std::cerr << "TypeError: tuple indices must be integers\n";
    std::exit(1);
  }

  auto index = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(index, static_cast<int64_t>(0), tuple->length())) {
    std::cerr << "IndexError: tuple index out of range\n";
    std::exit(1);
  }
  return tuple->Get(index);
}

void PyTupleKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                       Handle<PyObject> subscr,
                                       Handle<PyObject> value) {
  std::cerr << "TypeError: 'tuple' object does not support item assignment\n";
  std::exit(1);
}

void PyTupleKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                     Handle<PyObject> subscr) {
  std::cerr << "TypeError: 'tuple' object doesn't support item deletion\n";
  std::exit(1);
}

Tagged<PyBoolean> PyTupleKlass::Virtual_Contains(Handle<PyObject> self,
                                                 Handle<PyObject> target) {
  auto tuple = Handle<PyTuple>::cast(self);
  for (auto i = 0; i < tuple->length(); ++i) {
    if (IsPyTrue(PyObject::Equal(tuple->Get(i), target))) {
      return Isolate::Current()->py_true_object();
    }
  }
  return Isolate::Current()->py_false_object();
}

Tagged<PyBoolean> PyTupleKlass::Virtual_Equal(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  if (!IsPyTuple(*other)) {
    return Isolate::Current()->py_false_object();
  }

  auto tuple1 = Handle<PyTuple>::cast(self);
  auto tuple2 = Handle<PyTuple>::cast(other);

  if (tuple1->length() != tuple2->length()) {
    return Isolate::Current()->py_false_object();
  }

  for (auto i = 0; i < tuple1->length(); ++i) {
    if (IsPyFalse(PyObject::Equal(tuple1->Get(i), tuple2->Get(i)))) {
      return Isolate::Current()->py_false_object();
    }
  }

  return Isolate::Current()->py_true_object();
}

Handle<PyObject> PyTupleKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyTupleIterator::NewInstance(Handle<PyTuple>::cast(self));
}

size_t PyTupleKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(IsPyTuple(self));
  return PyTuple::ComputeObjectSize(Tagged<PyTuple>::cast(self)->length());
}

void PyTupleKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyTuple(self));
  auto tuple = Tagged<PyTuple>::cast(self);
  v->VisitPointers(tuple->data(), tuple->data() + tuple->length());
}

}  // namespace saauso::internal
