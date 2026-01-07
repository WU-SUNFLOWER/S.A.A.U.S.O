// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-smi-klass.h"

#include <cstdio>

#include "heap/heap.h"
#include "objects/py-oddballs.h"
#include "objects/py-float.h"
#include "objects/py-smi.h"
#include "objects/py-string.h"
#include "py-object.h"
#include "runtime/universe.h"
#include "utils/utils.h"

namespace saauso::internal {

PySmiKlass* PySmiKlass::instance_ = nullptr;

// static
PySmiKlass* PySmiKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PySmiKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::Initialize() {
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
  set_name(PyString::NewInstance("int"));
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%lld", PySmi::Cast(*self)->value());
}

// static
Handle<PyObject> PySmiKlass::Virtual_Add(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

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
Handle<PyObject> PySmiKlass::Virtual_Sub(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

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
Handle<PyObject> PySmiKlass::Virtual_Mul(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

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
Handle<PyObject> PySmiKlass::Virtual_Div(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

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
Handle<PyObject> PySmiKlass::Virtual_Mod(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

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
PyBoolean* PySmiKlass::Virtual_Greater(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  assert(self->IsPySmi());

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
PyBoolean* PySmiKlass::Virtual_Less(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  assert(self->IsPySmi());

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
PyBoolean* PySmiKlass::Virtual_Equal(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  assert(self->IsPySmi());

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
PyBoolean* PySmiKlass::Virtual_NotEqual(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  assert(self->IsPySmi());

  return Universe::ToPyBoolean(!Virtual_Equal(self, other)->IsPyTrue());
}

// static
PyBoolean* PySmiKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  assert(self->IsPySmi());

  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Equal(self, other)->IsPyTrue());
  return Universe::ToPyBoolean(v);
}

// static
PyBoolean* PySmiKlass::Virtual_LessEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(self->IsPySmi());

  bool v = (Virtual_Greater(self, other)->IsPyTrue() ||
            Virtual_Less(self, other)->IsPyFalse());

  return Universe::ToPyBoolean(v);
}

}  // namespace saauso::internal
