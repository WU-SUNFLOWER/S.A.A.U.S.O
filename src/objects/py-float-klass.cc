// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float-klass.h"

#include <cstdio>
#include <cstdlib>

#include "src/heap/heap.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

double ExtractValue(Handle<PyObject> object) {
  if (IsPyFloat(object)) {
    return Handle<PyFloat>::cast(object)->value();
  }

  if (IsPySmi(object)) {
    return PySmi::ToInt(Handle<PySmi>::cast(object));
  }

  std::printf(
      "TypeError: unsupported operand type(s) for +: 'float' and '%.*s'",
      static_cast<int>(PyObject::GetKlass(object)->name()->length()),
      PyObject::GetKlass(object)->name()->buffer());
  std::exit(1);

  return 0;
}

}  // namespace

////////////////////////////////////////////////////////////////////

Tagged<PyFloatKlass> PyFloatKlass::instance_(nullptr);

// static
Tagged<PyFloatKlass> PyFloatKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyFloatKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyFloatKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

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
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("float"));
}

void PyFloatKlass::Finalize() {
  instance_ = Tagged<PyFloatKlass>::Null();
}

////////////////////////////////////////////////////////////////////

void PyFloatKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%g", PyFloat::cast(*self)->value());
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Add(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value + other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Sub(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value - other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Mul(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value * other_value);
}

// static
Handle<PyObject> PyFloatKlass::Virtual_Div(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
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
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    std::printf("ZeroDivisionError: float modulo");
    std::exit(1);
  }
  return PyFloat::NewInstance(PythonMod(self_value, other_value));
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_Greater(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value > other_value);
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_Less(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value < other_value);
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_Equal(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value == other_value);
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Universe::ToPyBoolean(self_value != other_value);
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                                     Handle<PyObject> other) {
  assert(IsPyFloat(self));
  bool v = (IsPyTrue(Virtual_Greater(self, other)) ||
            IsPyTrue(Virtual_Equal(self, other)));
  return Universe::ToPyBoolean(v);
}

// static
Tagged<PyBoolean> PyFloatKlass::Virtual_LessEqual(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  assert(IsPyFloat(self));
  bool v = (IsPyTrue(Virtual_Less(self, other)) ||
            IsPyTrue(Virtual_Equal(self, other)));
  return Universe::ToPyBoolean(v);
}

// static
size_t PyFloatKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyFloat));
}

// static
void PyFloatKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  // PyFloat没有内部数据，什么都不用做
}

}  // namespace saauso::internal
