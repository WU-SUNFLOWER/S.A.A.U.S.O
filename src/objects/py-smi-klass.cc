// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi-klass.h"

#include <cinttypes>
#include <cmath>
#include <cstdio>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-smi.h"
#include "src/utils/maybe.h"
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
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.construct_instance = &Virtual_ConstructInstance;
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

MaybeHandle<PyObject> PySmiKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PySmiKlass::GetInstance());
  return Runtime_NewSmi(args, kwargs);
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("%" PRId64, PySmi::cast(*self).value());
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Add(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyFloat(other)) {
    double value = self_value + Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for +: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Sub(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyFloat(other)) {
    double value = self_value - Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for -: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Mul(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyFloat(other)) {
    double value = self_value * Handle<PyFloat>::cast(other)->value();
    return PyFloat::NewInstance(value);
  }
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for *: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Div(Handle<PyObject> self,
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
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for /: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PySmiKlass::Virtual_FloorDiv(Handle<PyObject> self,
                                                   Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPySmi(other)) {
    int64_t other_value = PySmi::cast(*other).value();
    if (other_value == 0) {
      Runtime_ThrowError(ExceptionType::kZeroDivisionError,
                         "integer division or modulo by zero");
      return kNullMaybeHandle;
    }
    return handle(PySmi::FromInt(PythonFloorDivide(self_value, other_value)));
  }
  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    if (other_value == 0) {
      Runtime_ThrowError(ExceptionType::kZeroDivisionError,
                         "float floor division by zero");
      return kNullMaybeHandle;
    }
    return PyFloat::NewInstance(PythonFloorDivide(self_value, other_value));
  }
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for //: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Mod(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    double value = PythonMod(self_value, other_value);
    return PyFloat::NewInstance(value);
  }
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for %%: 'int' and '%s'",
                      PyObject::GetKlass(other)->name()->buffer());
  return kNullMaybeHandle;
}

Maybe<uint64_t> PySmiKlass::Virtual_Hash(Handle<PyObject> self) {
  return Maybe<uint64_t>(PySmi::cast(*self).value());
}

Maybe<bool> PySmiKlass::Virtual_Greater(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return Maybe<bool>(self_value > other_value);
  }
  auto other_name = PyObject::GetKlass(other)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "'>' not supported between instances of 'int' and '%s'\n",
                      other_name->buffer());
  return kNullMaybe;
}

Maybe<bool> PySmiKlass::Virtual_Less(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  assert(IsPySmi(self));

  int64_t self_value = PySmi::cast(*self).value();

  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return Maybe<bool>(self_value < other_value);
  }
  auto other_name = PyObject::GetKlass(other)->name();
  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "'<' not supported between instances of 'int' and '%s'\n",
                      other_name->buffer());
  return kNullMaybe;
}

Maybe<bool> PySmiKlass::Virtual_Equal(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  assert(IsPySmi(self));
  int64_t self_value = PySmi::cast(*self).value();
  if (IsPyBoolean(other)) {
    return Maybe<bool>(self_value ==
                       (Handle<PyBoolean>::cast(other)->value() ? 1 : 0));
  }
  if (IsPyFloat(other)) {
    double other_value = Handle<PyFloat>::cast(other)->value();
    return Maybe<bool>(self_value == other_value);
  }
  if (IsPySmi(other)) {
    return Maybe<bool>(self_value == PySmi::cast(*other).value());
  }
  return Maybe<bool>(false);
}

Maybe<bool> PySmiKlass::Virtual_NotEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  assert(IsPySmi(self));

  bool is_equal;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), is_equal,
                             Virtual_Equal(self, other));

  return Maybe<bool>(!is_equal);
}

Maybe<bool> PySmiKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(IsPySmi(self));

  auto* isolate = Isolate::Current();
  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Virtual_Greater(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> PySmiKlass::Virtual_LessEqual(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  assert(IsPySmi(self));

  auto* isolate = Isolate::Current();
  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Virtual_Less(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(lt || eq);
}

}  // namespace saauso::internal
