// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float-klass.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
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
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/utils/number-conversion.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

// 从 PyObject 提取数值用于 float 运算；类型不支持时抛出 TypeError。
double ExtractValue(Handle<PyObject> object) {
  if (IsPyFloat(object)) {
    return Handle<PyFloat>::cast(object)->value();
  }

  if (IsPySmi(object)) {
    return PySmi::ToInt(Handle<PySmi>::cast(object));
  }

  Runtime_ThrowErrorf(ExceptionType::kTypeError,
                      "unsupported operand type(s) for +: 'float' and '%s'",
                      PyObject::GetKlass(object)->name()->buffer());
  return 0;
}

}  // namespace

////////////////////////////////////////////////////////////////////

// static
Tagged<PyFloatKlass> PyFloatKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyFloatKlass> instance = isolate->py_float_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyFloatKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_float_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyFloatKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  // Python中float类型只有默认的__new__而没有__init__
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.add_ = &Virtual_Add;
  vtable_.sub_ = &Virtual_Sub;
  vtable_.mul_ = &Virtual_Mul;
  vtable_.div_ = &Virtual_Div;
  vtable_.floor_div_ = &Virtual_FloorDiv;
  vtable_.mod_ = &Virtual_Mod;
  vtable_.greater_ = &Virtual_Greater;
  vtable_.less_ = &Virtual_Less;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.ge_ = &Virtual_GreaterEqual;
  vtable_.le_ = &Virtual_LessEqual;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyFloatKlass::Initialize(Isolate* isolate) {
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

  // 设置类名
  set_name(PyString::NewInstance("float"));

  return JustVoid();
}

void PyFloatKlass::Finalize(Isolate* isolate) {
  isolate->set_py_float_klass(Tagged<PyFloatKlass>::null());
}

////////////////////////////////////////////////////////////////////

MaybeHandle<PyObject> PyFloatKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(receiver_type->own_klass() == PyFloatKlass::GetInstance());

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "float() takes no keyword arguments\n");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc == 0) {
    return isolate->factory()->NewPyFloat(0.0);
  }

  if (argc != 1) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "float() takes at most 1 argument (%" PRId64 " given)\n", argc);
    return kNullMaybeHandle;
  }

  Handle<PyObject> value = pos_args->Get(0);
  if (IsPyFloat(value)) {
    return value;
  }
  if (IsPySmi(value)) {
    return isolate->factory()->NewPyFloat(
        PySmi::ToInt(Handle<PySmi>::cast(value)));
  }
  if (IsPyBoolean(value)) {
    bool v = Tagged<PyBoolean>::cast(*value)->value();
    return isolate->factory()->NewPyFloat(v ? 1.0 : 0.0);
  }
  if (IsPyString(value)) {
    auto s = Handle<PyString>::cast(value);
    std::string_view text(s->buffer(), static_cast<size_t>(s->length()));
    std::optional<double> parsed = StringToDouble(text);
    if (!parsed) {
      Runtime_ThrowErrorf(ExceptionType::kValueError,
                          "could not convert string to float: '%s'\n",
                          s->buffer());
      return kNullMaybeHandle;
    }
    return isolate->factory()->NewPyFloat(*parsed);
  }

  auto type_name = PyObject::GetKlass(value)->name();
  Runtime_ThrowErrorf(
      ExceptionType::kTypeError,
      "float() argument must be a string or a real number, not '%s'\n",
      type_name->buffer());
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Repr(Isolate* isolate,
                                                 Handle<PyObject> self) {
  return PyString::FromPyFloat(Handle<PyFloat>::cast(self));
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Str(Isolate* isolate,
                                                Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Add(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value + other_value);
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Sub(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value - other_value);
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Mul(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return PyFloat::NewInstance(self_value * other_value);
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Div(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    Runtime_ThrowError(ExceptionType::kZeroDivisionError,
                       "float division by zero");
    return kNullMaybeHandle;
  }
  return PyFloat::NewInstance(self_value / other_value);
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_FloorDiv(Handle<PyObject> self,
                                                     Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    Runtime_ThrowError(ExceptionType::kZeroDivisionError,
                       "float floor division by zero");
    return kNullMaybeHandle;
  }
  return PyFloat::NewInstance(PythonFloorDivide(self_value, other_value));
}

MaybeHandle<PyObject> PyFloatKlass::Virtual_Mod(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  if (other_value == 0) {
    Runtime_ThrowError(ExceptionType::kZeroDivisionError, "float modulo");
    return kNullMaybeHandle;
  }
  return PyFloat::NewInstance(PythonMod(self_value, other_value));
}

Maybe<bool> PyFloatKlass::Virtual_Greater(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Maybe<bool>(self_value > other_value);
}

Maybe<bool> PyFloatKlass::Virtual_Less(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Maybe<bool>(self_value < other_value);
}

Maybe<bool> PyFloatKlass::Virtual_Equal(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Maybe<bool>(self_value == other_value);
}

Maybe<bool> PyFloatKlass::Virtual_NotEqual(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  assert(IsPyFloat(self));
  double self_value = Handle<PyFloat>::cast(self)->value();
  double other_value = ExtractValue(other);
  return Maybe<bool>(self_value != other_value);
}

Maybe<bool> PyFloatKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                               Handle<PyObject> other) {
  assert(IsPyFloat(self));

  auto* isolate [[maybe_unused]] = Isolate::Current();
  bool gt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, gt, Virtual_Greater(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(gt || eq);
}

Maybe<bool> PyFloatKlass::Virtual_LessEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  assert(IsPyFloat(self));

  auto* isolate [[maybe_unused]] = Isolate::Current();
  bool lt, eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, lt, Virtual_Less(self, other));
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(self, other));

  return Maybe<bool>(lt || eq);
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
