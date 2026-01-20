// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#include <cassert>

#include "py-object.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/mark-word.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

MarkWord PyObject::GetMarkWord(Tagged<PyObject> object) {
  assert(IsHeapObject(object));
  return object->mark_word_;
}

void PyObject::SetMapWordForwarded(Tagged<PyObject> object,
                                   Tagged<PyObject> target) {
  object->mark_word_ = MarkWord::FromForwardingAddress(target);
}

Tagged<Klass> PyObject::GetKlass(Tagged<PyObject> object) {
  // 特化：Smi使用PySmiKlass，使得它表现得像一个标准的Python对象
  if (IsPySmi(object)) {
    return PySmiKlass::GetInstance();
  }

  assert(IsHeapObject(object));
  assert(!object->mark_word_.ToKlass().IsNull());

  return object->mark_word_.ToKlass();
}

Tagged<Klass> PyObject::GetKlass(Handle<PyObject> object) {
  return GetKlass(*object);
}

void PyObject::SetKlass(Tagged<PyObject> object, Tagged<Klass> klass) {
  assert(!klass.IsNull());
  assert(IsHeapObject(object));
  object->mark_word_ = MarkWord::FromKlass(klass);
}

void PyObject::SetKlass(Handle<PyObject> object, Tagged<Klass> klass) {
  SetKlass(*object, klass);
}

Handle<PyDict> PyObject::GetProperties(Handle<PyObject> object) {
  return GetProperties(*object);
}

Handle<PyDict> PyObject::GetProperties(Tagged<PyObject> object) {
  if (!IsHeapObject(object) || object->properties_.IsNull()) {
    return Handle<PyDict>::Null();
  }
  return handle(Tagged<PyDict>::cast(object->properties_));
}

void PyObject::SetProperties(Tagged<PyObject> object,
                             Tagged<PyDict> properties) {
  // properties不能是smi！
  assert(properties.IsNull() || IsHeapObject(properties));

  object->properties_ = properties;
  WRITE_BARRIER(object, &object->properties_, properties);
}

///////////////////////////////////////////////////////////////////
// 类型判断工具函数 开始

#define IMPL_PY_CHECKER_WITH_HANDLE_ARG(name) \
  bool Is##name(Handle<PyObject> object) {    \
    return Is##name(*object);                 \
  }

#define IMPL_PY_CHECKER_BY_KLASS(name)         \
  bool Is##name(Tagged<PyObject> object) {     \
    return PyObject::GetKlass(object).ptr() == \
           name##Klass::GetInstance().ptr();   \
  }                                            \
  IMPL_PY_CHECKER_WITH_HANDLE_ARG(name)

// 实现IsPyFloat、IsPyString、IsPyList等一系列函数
IMPL_PY_CHECKER_BY_KLASS(PyTypeObject)
IMPL_PY_CHECKER_BY_KLASS(PyString)
IMPL_PY_CHECKER_BY_KLASS(PyFloat)
IMPL_PY_CHECKER_BY_KLASS(PyBoolean)
IMPL_PY_CHECKER_BY_KLASS(PyNone)
IMPL_PY_CHECKER_BY_KLASS(PyCodeObject)
IMPL_PY_CHECKER_BY_KLASS(PyList)
IMPL_PY_CHECKER_BY_KLASS(PyTuple)
IMPL_PY_CHECKER_BY_KLASS(PyDict)
IMPL_PY_CHECKER_BY_KLASS(FixedArray)
IMPL_PY_CHECKER_BY_KLASS(MethodObject)
#undef IMPL_PY_CHECKER_BY_KLASS

bool IsPyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == PyFunctionKlass::GetInstance() ||
         PyObject::GetKlass(object) == NativeFunctionKlass::GetInstance();
}

bool IsNormalPyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == PyFunctionKlass::GetInstance();
}

bool IsNativePyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == NativeFunctionKlass::GetInstance();
}

bool IsPySmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

bool IsPyTrue(Tagged<PyObject> object) {
  return object.ptr() ==
         Tagged<PyObject>(Isolate::Current()->py_true_object()).ptr();
}

bool IsPyFalse(Tagged<PyObject> object) {
  return object.ptr() ==
         Tagged<PyObject>(Isolate::Current()->py_false_object()).ptr();
}

bool IsHeapObject(Tagged<PyObject> object) {
  return !object.IsNull() && !IsPySmi(object);
}

bool IsPyNativeFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object).ptr() ==
         NativeFunctionKlass::GetInstance().ptr();
}

bool IsGcAbleObject(Tagged<PyObject> object) {
  if (IsPySmi(object)) {
    return false;
  }
  if (PyObject::GetMarkWord(object).IsForwardingAddress()) {
    return true;
  }
  return !IsPyBoolean(object) && !IsPyNone(object);
}

IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(NormalPyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(NativePyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PySmi)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTrue)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFalse)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(HeapObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyNativeFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(GcAbleObject)
#undef IMPL_PY_CHECKER_WITH_HANDLE_ARG

///////////////////////////////////////////////////////////////////
// 多态虚方法入口 开始

// python virtual function
void PyObject::Print(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.print);
  GetKlass(*self)->vtable_.print(self);
}

// python virtual function
Handle<PyObject> PyObject::Add(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() +
                                           PySmi::cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.add);
  return GetKlass(*self)->vtable_.add(self, other).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Sub(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() -
                                           PySmi::cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.sub);
  return GetKlass(*self)->vtable_.sub(self, other).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Mul(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() *
                                           PySmi::cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mul);
  return GetKlass(*self)->vtable_.mul(self, other).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Div(Handle<PyObject> self, Handle<PyObject> other) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.div);
  return GetKlass(*self)->vtable_.div(self, other).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Mod(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    auto result =
        PythonMod(PySmi::cast(*self).value(), PySmi::cast(*other).value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mod);
  return GetKlass(*self)->vtable_.mod(self, other).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Len(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.len);
  return GetKlass(*self)->vtable_.len(self).EscapeFrom(&scope);
}

// python virtual function
Tagged<PyBoolean> PyObject::Greater(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() >
                                PySmi::cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.greater);
  return GetKlass(*self)->vtable_.greater(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::Less(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() <
                                PySmi::cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.less);
  return GetKlass(*self)->vtable_.less(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() ==
                                PySmi::cast(*other).value());
  }

  // 内联Fast Path：直接比较内存地址
  if (self.is_identical_to(other)) {
    return Isolate::Current()->py_true_object();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.equal);
  return GetKlass(*self)->vtable_.equal(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::NotEqual(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() !=
                                PySmi::cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.not_equal);
  return GetKlass(*self)->vtable_.not_equal(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::GreaterEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() >=
                                PySmi::cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.ge);
  return GetKlass(*self)->vtable_.ge(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::LessEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Isolate::ToPyBoolean(PySmi::cast(*self).value() <=
                                PySmi::cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.le);
  return GetKlass(*self)->vtable_.le(self, other);
}

// python virtual function
Handle<PyObject> PyObject::GetAttr(Handle<PyObject> self,
                                   Handle<PyObject> attr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.getattr);
  return GetKlass(*self)->vtable_.getattr(self, attr_name).EscapeFrom(&scope);
}

// python virtual function
void PyObject::SetAttr(Handle<PyObject> self,
                       Handle<PyObject> attr_name,
                       Handle<PyObject> attr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.setattr);
  GetKlass(*self)->vtable_.setattr(self, attr_name, attr_value);
}

// python virtual function
Handle<PyObject> PyObject::Subscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.subscr);
  return GetKlass(*self)->vtable_.subscr(self, subscr_name).EscapeFrom(&scope);
}

// python virtual function
void PyObject::StoreSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name,
                           Handle<PyObject> subscr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.store_subscr);
  GetKlass(*self)->vtable_.store_subscr(self, subscr_name, subscr_value);
}

// python virtual function
Tagged<PyBoolean> PyObject::Contains(Handle<PyObject> self,
                                     Handle<PyObject> target) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.contains);
  return GetKlass(*self)->vtable_.contains(self, target);
}

// python virtual function
Handle<PyObject> PyObject::Iter(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.iter);
  return GetKlass(*self)->vtable_.iter(self).EscapeFrom(&scope);
}

// python virtual function
Handle<PyObject> PyObject::Next(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.next);
  return GetKlass(*self)->vtable_.next(self).EscapeFrom(&scope);
}

// python virtual function
void PyObject::DeletSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.del_subscr);
  GetKlass(*self)->vtable_.del_subscr(self, subscr_name);
}

// python virtual function
uint64_t PyObject::Hash(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.hash);
  return GetKlass(*self)->vtable_.hash(self);
}

Handle<PyObject> PyObject::Call(Handle<PyObject> self,
                                Handle<PyObject> args,
                                Handle<PyObject> kwargs) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.call);
  return GetKlass(*self)->vtable_.call(self, args, kwargs).EscapeFrom(&scope);
}

// python virtual function
size_t PyObject::GetInstanceSize(Tagged<PyObject> self) {
  return GetKlass(self)->vtable_.instance_size(self);
}

// python virtual function
void PyObject::Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  v->VisitPointer(&self->properties_);

  assert(GetKlass(self)->vtable_.iterate);
  GetKlass(self)->vtable_.iterate(self, v);
}

}  // namespace saauso::internal
