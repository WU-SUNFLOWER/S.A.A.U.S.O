// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float-klass.h"

#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-float.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-object.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"


namespace saauso::internal {

namespace {

double ExtractValue(Handle<PyObject> object) {
  if (object->IsPyFloat()) {
    return Handle<PyFloat>::Cast(object)->value();
  }

  if (object->IsPySmi()) {
    return Handle<PySmi>::Cast(object)->value();
  }

  std::printf(
      "TypeError: unsupported operand type(s) for +: 'float' and '%.*s'",
      static_cast<int>(object->klass()->name()->length()),
      object->klass()->name()->buffer());
  std::exit(1);

  return 0;
}

}  // namespace

////////////////////////////////////////////////////////////////////

PyFloatKlass* PyFloatKlass::instance_ = nullptr;

// static
PyFloatKlass* PyFloatKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyFloatKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyFloatKlass::Initialize() {
  // 初始化虚函数表
  vtable_.add = &Virtual_Add;
  vtable_.sub = &Virtual_Sub;
  vtable_.mul = &Virtual_Mul;
  vtable_.div = &Virtual_Div;
  vtable_.mod = &Virtual_Mod;
  vtable_.greater = &Virtual_Greater;
  vtable_.less = &Virtual_Less;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.le = &Virtual_LessEqual;
  vtable_.print = &Virtual_Print;

  // 设置类名
  set_name(PyString::NewInstance("float"));
}

////////////////////////////////////////////////////////////////////

void PyFloatKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%g", PyFloat::Cast(*self)->value());
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Add(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value + other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Sub(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value - other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Mul(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value * other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Div(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    std::printf("ZeroDivisionError: float division by zero");
    std::exit(1);
  }
  return PyFloat::NewInstance(self_value / other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Mod(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    std::printf("ZeroDivisionError: float modulo");
    std::exit(1);
  }
  return PyFloat::NewInstance(PythonMod(self_value, other_value));
}

// static
PyBoolean* PyFloatKlass::Virtual_Greater(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value > other_value);
}

// static
PyBoolean* PyFloatKlass::Virtual_Less(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value < other_value);
}

// static
PyBoolean* PyFloatKlass::Virtual_Equal(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value == other_value);
}

// static
PyBoolean* PyFloatKlass::Virtual_NotEqual(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  assert(self->IsPyFloat());
  double self_value = Handle<PyFloat>::Cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value != other_value);
}

// static
PyBoolean* PyFloatKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(self->IsPySmi());
  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Equal(self, other)->IsPyTrue());
  return Universe::ToPyBoolean(v);
}

// static
PyBoolean* PyFloatKlass::Virtual_LessEqual(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPySmi());
  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Less(self, other)->IsPyFalse());
  return Universe::ToPyBoolean(v);
}

}  // namespace saauso::internal
