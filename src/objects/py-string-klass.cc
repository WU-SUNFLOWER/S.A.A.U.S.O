// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-string-klass.h"

#include <cstdio>
#include <cstdlib>

#include "handles/handles.h"
#include "heap/heap.h"
#include "objects/py-smi.h"
#include "objects/py-string.h"
#include "runtime/universe.h"
#include "utils/utils.h"

namespace saauso::internal {

PyStringKlass* PyStringKlass::instance_ = nullptr;

// static
PyStringKlass* PyStringKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyStringKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyStringKlass::Initialize() {
  // 初始化虚函数表
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.less = &Virtual_Less;
  vtable_.greater = &Virtual_Greater;
  vtable_.le = &Virtual_LessEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.add = &Virtual_Add;
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;

  // 设置类名
  set_name(PyString::NewInstance("str"));
}

Handle<PyObject> PyStringKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyString>::Cast(self)->length()));
}

PyBoolean* PyStringKlass::Virtual_Equal(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2));
}

PyBoolean* PyStringKlass::Virtual_NotEqual(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(!s1->IsEqualTo(*s2));
}

PyBoolean* PyStringKlass::Virtual_Less(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(s1->IsLessThan(*s2));
}

PyBoolean* PyStringKlass::Virtual_Greater(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(s1->IsGreaterThan(*s2));
}

PyBoolean* PyStringKlass::Virtual_LessEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2) || s1->IsLessThan(*s2));
}

PyBoolean* PyStringKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                               Handle<PyObject> other) {
  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2) || s1->IsGreaterThan(*s2));
}

Handle<PyObject> PyStringKlass::Virtual_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  auto s = Handle<PyString>::Cast(self);

  auto decoded_subscr = Handle<PySmi>::Cast(subscr)->value();
  if (!InRangeWithRightOpen(decoded_subscr, 0ll, s->length())) [[unlikely]] {
    std::printf("IndexError: string index out of range");
    std::exit(1);
  }

  return PyString::NewInstance(s->buffer() + decoded_subscr, 1);
}

Handle<PyObject> PyStringKlass::Virtual_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  if (!other->IsPyString()) [[unlikely]] {
    std::printf("TypeError: can only concatenate str (not \"%.*s\") to str",
                static_cast<int>(other->klass()->name()->length()),
                other->klass()->name()->buffer());
    std::exit(1);
  }

  auto s1 = Handle<PyString>::Cast(self);
  auto s2 = Handle<PyString>::Cast(other);
  return s1->Append(s2);
}

void PyStringKlass::Virtual_Print(Handle<PyObject> self) {
  auto s = Handle<PyString>::Cast(self);
  std::printf("%.*s", static_cast<int>(s->length()), s->buffer());
}

size_t PyStringKlass::Virtual_InstanceSize(PyObject* self) {
  assert(self->IsPyString());
  return sizeof(PyString);
}

}  // namespace saauso::internal
