// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi-klass.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "src/heap/heap.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PySmiKlass> PySmiKlass::instance_(nullptr);

// static
Tagged<PySmiKlass> PySmiKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PySmiKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.add = &Virtual_Add;
  vtable_.sub = &Virtual_Sub;
  vtable_.mul = &Virtual_Mul;
  vtable_.div = &Virtual_Div;
  vtable_.mod = &Virtual_Mod;
  vtable_.hash = &Virtual_Hash;
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

void PySmiKlass::Finalize() {
  instance_ = Tagged<PySmiKlass>::Null();
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%" PRId64, PySmi::Cast(*self).value());
}

// static
Handle<PyObject> PySmiKlass::Virtual_Add(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value + Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::printf("TypeError: unsupported operand type(s) for +: 'int' and '%.*s'",
              static_cast<int>(PyObject::GetKlass(other)->name()->length()),
              PyObject::GetKlass(other)->name()->buffer());
  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  std::exit(1);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Sub(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value - Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::printf("TypeError: unsupported operand type(s) for -: 'int' and '%.*s'",
              static_cast<int>(PyObject::GetKlass(other)->name()->length()),
              PyObject::GetKlass(other)->name()->buffer());
  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  std::exit(1);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Mul(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value * Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::printf("TypeError: unsupported operand type(s) for *: 'int' and '%.*s'",
              static_cast<int>(PyObject::GetKlass(other)->name()->length()),
              PyObject::GetKlass(other)->name()->buffer());
  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  std::exit(1);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Div(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double value =
        static_cast<double>(self_value) / Handle<PyFloat>::Cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::printf("TypeError: unsupported operand type(s) for /: 'int' and '%.*s'",
              static_cast<int>(PyObject::GetKlass(other)->name()->length()),
              PyObject::GetKlass(other)->name()->buffer());
  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  std::exit(1);

  return Handle<PyObject>::Null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Mod(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    double value = PythonMod(self_value, other_value);
    return PyFloat::NewInstance(value);
  }

  std::printf("TypeError: unsupported operand type(s) for %%: 'int' and '%.*s'",
              static_cast<int>(PyObject::GetKlass(other)->name()->length()),
              PyObject::GetKlass(other)->name()->buffer());

  // TODO: 现在虚拟机里还没有错误处理系统，我们先暂时让它直接崩溃掉
  std::exit(1);

  return Handle<PyObject>::Null();
}

// static
uint64_t PySmiKlass::Virtual_Hash(Handle<PyObject> self) {
  // 对于负数，会被hash成一个巨大的正数
  return PySmi::Cast(*self).value();
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_Greater(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value > other_value);
  }

  return Universe::py_false_object_;
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_Less(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value < other_value);
  }

  return Universe::py_false_object_;
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_Equal(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::Cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::Cast(other)->value();
    return Universe::ToPyBoolean(self_value == other_value);
  }

  return Universe::py_false_object_;
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_NotEqual(Handle<PyObject> self,
                                               Handle<PyObject> other) {
  assert(IsPySmi(self));

  return Virtual_Equal(self, other)->Reverse();
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                                   Handle<PyObject> other) {
  assert(IsPySmi(self));

  bool v = (IsPyTrue(Virtual_Greater(self, other)) ||
            IsPyTrue(Virtual_Equal(self, other)));
  return Universe::ToPyBoolean(v);
}

// static
Tagged<PyBoolean> PySmiKlass::Virtual_LessEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPySmi(self));

  bool v = (IsPyTrue(Virtual_Less(self, other)) ||
            IsPyTrue(Virtual_Equal(self, other)));

  return Universe::ToPyBoolean(v);
}

}  // namespace saauso::internal
