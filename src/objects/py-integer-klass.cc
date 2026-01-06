// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-integer-klass.h"

#include "heap/heap.h"
#include "objects/py-boolean.h"
#include "objects/py-float.h"
#include "objects/py-integer.h"
#include "objects/py-smi.h"
#include "objects/py-string.h"
#include "py-object.h"
#include "runtime/universe.h"
#include "utils/utils.h"

namespace saauso::internal {

PyIntegerKlass* PyIntegerKlass::instance_ = nullptr;

// static
PyIntegerKlass* PyIntegerKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyIntegerKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyIntegerKlass::Initialize() {
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

  // 设置类名
  set_name(PyString::NewInstance("int"));
}

////////////////////////////////////////////////////////////////////

// static
Handle<PyObject> PyIntegerKlass::Virtual_Add(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double value = self_value + Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PyIntegerKlass::Virtual_Sub(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double value = self_value - Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PyIntegerKlass::Virtual_Mul(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double value = self_value * Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PyIntegerKlass::Virtual_Div(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double value =
        static_cast<double>(self_value) / Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PyIntegerKlass::Virtual_Mod(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    double value = PythonMod(self_value, other_value);
    return PyFloat::NewInstance(value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return Handle<PyObject>::Null();
}

// static
PyBoolean* PyIntegerKlass::Virtual_Greater(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value > other_value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);
  return nullptr;
}

// static
PyBoolean* PyIntegerKlass::Virtual_Less(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value < other_value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);

  return nullptr;
}

// static
PyBoolean* PyIntegerKlass::Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPyInteger());

  int64_t self_value = PySmi::Cast(*self)->value();

  if (other->IsPyFloat()) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value == other_value);
  }

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  assert(0);
  return nullptr;
}

// static
PyBoolean* PyIntegerKlass::Virtual_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  assert(self->IsPyInteger());

  return Universe::ToPyBoolean(!Virtual_Equal(self, other)->IsPyTrue());
}

// static
PyBoolean* PyIntegerKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(self->IsPyInteger());

  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Equal(self, other)->IsPyTrue());
  return Universe::ToPyBoolean(v);
}

// static
PyBoolean* PyIntegerKlass::Virtual_LessEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(self->IsPyInteger());

  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Less(self, other)->IsPyFalse());

  return Universe::ToPyBoolean(v);
}

}  // namespace saauso::internal
