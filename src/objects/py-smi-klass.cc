// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi-klass.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// static
Tagged<PySmiKlass> PySmiKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PySmiKlass> instance = isolate->py_smi_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PySmiKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_smi_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(this);

  vtable_.add = &Virtual_Add;
  vtable_.sub = &Virtual_Sub;
  vtable_.mul = &Virtual_Mul;
  vtable_.div = &Virtual_Div;
  vtable_.floor_div = &Virtual_FloorDiv;
  vtable_.mod = &Virtual_Mod;
  vtable_.hash = &Virtual_Hash;
  vtable_.greater = &Virtual_Greater;
  vtable_.less = &Virtual_Less;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.le = &Virtual_LessEqual;
  vtable_.print = &Virtual_Print;
}

void PySmiKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("int"));
}

void PySmiKlass::Finalize() {
  Isolate::Current()->set_py_smi_klass(Tagged<PySmiKlass>::null());
}

////////////////////////////////////////////////////////////////////

void PySmiKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%" PRId64, PySmi::cast(*self).value());
}

// static
Handle<PyObject> PySmiKlass::Virtual_Add(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value + Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::fprintf(stderr,
               "TypeError: unsupported operand type(s) for +: 'int' and '%.*s'",
               static_cast<int>(PyObject::GetKlass(other)->name()->length()),
               PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Sub(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value - Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::fprintf(stderr,
               "TypeError: unsupported operand type(s) for -: 'int' and '%.*s'",
               static_cast<int>(PyObject::GetKlass(other)->name()->length()),
               PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Mul(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double value = self_value * Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::fprintf(stderr,
               "TypeError: unsupported operand type(s) for *: 'int' and '%.*s'",
               static_cast<int>(PyObject::GetKlass(other)->name()->length()),
               PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Div(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPySmi(other)) {
    double value =
        static_cast<double>(self_value) / PySmi::cast(*other).value();
    return PyFloat::NewInstance(value);
  }

  if (IsPyFloat(other)) {
    double value =
        static_cast<double>(self_value) / Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }

  std::fprintf(stderr,
               "TypeError: unsupported operand type(s) for /: 'int' and '%.*s'",
               static_cast<int>(PyObject::GetKlass(other)->name()->length()),
               PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_FloorDiv(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPySmi(other)) {
    int64_t other_value = PySmi::cast(*other).value();
    if (other_value == 0) {
      std::fprintf(stderr,
                   "ZeroDivisionError: integer division or modulo by zero");
      std::exit(1);
    }
    return handle(PySmi::FromInt(PythonFloorDivide(self_value, other_value)));
  }

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    if (other_value == 0) {
      std::fprintf(stderr, "ZeroDivisionError: float floor division by zero");
      std::exit(1);
    }
    return PyFloat::NewInstance(PythonFloorDivide(self_value, other_value));
  }

  std::fprintf(
      stderr, "TypeError: unsupported operand type(s) for //: 'int' and '%.*s'",
      static_cast<int>(PyObject::GetKlass(other)->name()->length()),
      PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
Handle<PyObject> PySmiKlass::Virtual_Mod(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    double value = PythonMod(self_value, other_value);
    return PyFloat::NewInstance(value);
  }

  std::fprintf(
      stderr, "TypeError: unsupported operand type(s) for %%: 'int' and '%.*s'",
      static_cast<int>(PyObject::GetKlass(other)->name()->length()),
      PyObject::GetKlass(other)->name()->buffer());
  std::exit(1);

  return Handle<PyObject>::null();
}

// static
uint64_t PySmiKlass::Virtual_Hash(Handle<PyObject> self) {
  return PySmi::cast(*self).value();
}

// static
bool PySmiKlass::Virtual_Greater(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return self_value > other_value;
  }
  auto other_name = PyObject::GetKlass(other)->name();
  std::fprintf(stderr,
               "TypeError: '>' not supported between instances of 'int' and "
               "'%.*s'\n",
               static_cast<int>(other_name->length()), other_name->buffer());
  std::exit(1);
  return false;
}

// static
bool PySmiKlass::Virtual_Less(Handle<PyObject> self, Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return self_value < other_value;
  }
  auto other_name = PyObject::GetKlass(other)->name();
  std::fprintf(stderr,
               "TypeError: '<' not supported between instances of 'int' and "
               "'%.*s'\n",
               static_cast<int>(other_name->length()), other_name->buffer());
  std::exit(1);
  return false;
}

// static
bool PySmiKlass::Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return self_value == other_value;
  }
  return false;
}

// static
bool PySmiKlass::Virtual_NotEqual(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  assert(IsPySmi(self));

  return !Virtual_Equal(self, other);
}

// static
bool PySmiKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  assert(IsPySmi(self));

  return Virtual_Greater(self, other) || Virtual_Equal(self, other);
}

// static
bool PySmiKlass::Virtual_LessEqual(Handle<PyObject> self,
                                   Handle<PyObject> other) {
  assert(IsPySmi(self));

  return Virtual_Less(self, other) || Virtual_Equal(self, other);
}

}  // namespace saauso::internal
