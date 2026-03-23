// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi-klass.h"

#include <cinttypes>
#include <cmath>
#include <cstdio>

#include "include/saauso-maybe.h"
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

void PySmiKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  // Python中int类型只有默认的__new__而没有__init__
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.add_ = &Virtual_Add;
  vtable_.sub_ = &Virtual_Sub;
  vtable_.mul_ = &Virtual_Mul;
  vtable_.div_ = &Virtual_Div;
  vtable_.floor_div_ = &Virtual_FloorDiv;
  vtable_.mod_ = &Virtual_Mod;
  vtable_.hash_ = &Virtual_Hash;
  vtable_.greater_ = &Virtual_Greater;
  vtable_.less_ = &Virtual_Less;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.ge_ = &Virtual_GreaterEqual;
  vtable_.le_ = &Virtual_LessEqual;
}

Maybe<void> PySmiKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::NewInstance("int"));

  return JustVoid();
}

void PySmiKlass::Finalize(Isolate* isolate) {
  isolate->set_py_smi_klass(Tagged<PySmiKlass>::null());
}

////////////////////////////////////////////////////////////////////

MaybeHandle<PyObject> PySmiKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(receiver_type->own_klass() == PySmiKlass::GetInstance());
  return Runtime_NewSmi(args, kwargs);
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Repr(Isolate* isolate,
                                               Handle<PyObject> self) {
  return PyString::FromPySmi(Tagged<PySmi>::cast(*self));
}

MaybeHandle<PyObject> PySmiKlass::Virtual_Str(Isolate* isolate,
                                              Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
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

  auto* isolate [[maybe_unused]] = Isolate::Current();
  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Virtual_Greater(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> PySmiKlass::Virtual_LessEqual(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  assert(IsPySmi(self));

  auto* isolate [[maybe_unused]] = Isolate::Current();
  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Virtual_Less(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(lt || eq);
}

}  // namespace saauso::internal
