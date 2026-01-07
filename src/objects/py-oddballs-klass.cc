// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-oddballs-klass.h"

#include <cstdio>

#include "heap/heap.h"
#include "objects/py-float.h"
#include "objects/py-oddballs.h"
#include "objects/py-smi.h"
#include "objects/py-string.h"
#include "py-object.h"
#include "runtime/universe.h"
#include "utils/utils.h"

namespace saauso::internal {

PyBooleanKlass* PyBooleanKlass::instance_ = nullptr;
PyNoneKlass* PyNoneKlass::instance_ = nullptr;

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
PyBooleanKlass* PyBooleanKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyBooleanKlass::Initialize() {
  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;

  // 设置类名
  set_name(PyString::NewInstance("bool"));
}

// static
void PyBooleanKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf(Handle<PyBoolean>::Cast(self)->value() ? "True" : "False");
}

// static
PyBoolean* PyBooleanKlass::Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  bool v = Handle<PyBoolean>::Cast(self)->value();

  bool result = false;
  if (other->IsPySmi()) {
    result = Handle<PySmi>::Cast(other)->value() == v;
  } else if (other->IsPyFloat()) {
    result = Handle<PyFloat>::Cast(other)->value() == v;
  } else if (other->IsPyBoolean()) {
    result = Handle<PyBoolean>::Cast(other)->value() == v;
  }

  return Universe::ToPyBoolean(result);
}

// static
PyBoolean* PyBooleanKlass::Virtual_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
PyNoneKlass* PyNoneKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyNoneKlass::Initialize() {
  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;

  // 设置类名
  set_name(PyString::NewInstance("NoneType"));
}

// static
void PyNoneKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("None");
}

// static
PyBoolean* PyNoneKlass::Virtual_Equal(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  assert(self->IsPyNone());
  return Universe::ToPyBoolean(*self == *other);
}

// static
PyBoolean* PyNoneKlass::Virtual_NotEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

}  // namespace saauso::internal
