// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "py-object.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/cell-klass.h"
#include "src/objects/cell.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/mark-word.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/visitors.h"
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
  assert(!object->mark_word_.ToKlass().is_null());

  return object->mark_word_.ToKlass();
}

Tagged<Klass> PyObject::GetKlass(Handle<PyObject> object) {
  return GetKlass(*object);
}

void PyObject::SetKlass(Tagged<PyObject> object, Tagged<Klass> klass) {
  assert(!klass.is_null());
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
  if (!IsHeapObject(object) || object->properties_.is_null()) {
    return Handle<PyDict>::null();
  }
  return handle(Tagged<PyDict>::cast(object->properties_));
}

void PyObject::SetProperties(Tagged<PyObject> object,
                             Tagged<PyDict> properties) {
  // properties不能是smi！
  assert(properties.is_null() || IsHeapObject(properties));

  object->properties_ = properties;
  WRITE_BARRIER(object, &object->properties_, properties);
}

// static
Handle<PyObject> PyObject::AllocateRawPythonObject() {
  Handle<PyObject> object(Isolate::Current()->heap()->Allocate<PyObject>(
      Heap::AllocationSpace::kNewSpace));
  Handle<PyDict> properties = PyDict::NewInstance();
  SetProperties(*object, *properties);
  return object;
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
IMPL_PY_CHECKER_BY_KLASS(PyModule)
IMPL_PY_CHECKER_BY_KLASS(PyList)
IMPL_PY_CHECKER_BY_KLASS(PyListIterator)
IMPL_PY_CHECKER_BY_KLASS(PyTuple)
IMPL_PY_CHECKER_BY_KLASS(PyTupleIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDict)
IMPL_PY_CHECKER_BY_KLASS(PyDictKeys)
IMPL_PY_CHECKER_BY_KLASS(PyDictValues)
IMPL_PY_CHECKER_BY_KLASS(PyDictItems)
IMPL_PY_CHECKER_BY_KLASS(PyDictKeyIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDictItemIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDictValueIterator)
IMPL_PY_CHECKER_BY_KLASS(FixedArray)
IMPL_PY_CHECKER_BY_KLASS(MethodObject)
IMPL_PY_CHECKER_BY_KLASS(Cell)
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
  return !object.is_null() && !IsPySmi(object);
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

///////////////////////////////////////////////////////////////////////////
// 多态虚方法入口 开始

// python virtual function
void PyObject::Print(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().print);
  GetKlass(*self)->vtable().print(self);
}

// python virtual function
Handle<PyObject> PyObject::Add(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() +
                                           PySmi::cast(*other).value()));
  }

  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().add);
  return scope.Escape(GetKlass(*self)->vtable().add(self, other));
}

// python virtual function
Handle<PyObject> PyObject::Sub(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() -
                                           PySmi::cast(*other).value()));
  }

  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().sub);
  return scope.Escape(GetKlass(*self)->vtable().sub(self, other));
}

// python virtual function
Handle<PyObject> PyObject::Mul(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::cast(*self).value() *
                                           PySmi::cast(*other).value()));
  }

  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().mul);
  return scope.Escape(GetKlass(*self)->vtable().mul(self, other));
}

// python virtual function
Handle<PyObject> PyObject::Div(Handle<PyObject> self, Handle<PyObject> other) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().div);
  return scope.Escape(GetKlass(*self)->vtable().div(self, other));
}

Handle<PyObject> PyObject::FloorDiv(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  int64_t other_value;
  if (IsPySmi(self) && IsPySmi(other) &&
      (other_value = PySmi::cast(*other).value()) != 0) {
    int64_t self_value = PySmi::cast(*self).value();
    return Handle<PyObject>(
        PySmi::FromInt(PythonFloorDivide(self_value, other_value)));
  }

  EscapableHandleScope scope;

  auto floor_div = GetKlass(*self)->vtable().floor_div;
  if (floor_div == nullptr) [[unlikely]] {
    auto self_name = GetKlass(self)->name();
    auto other_name = GetKlass(other)->name();
    std::fprintf(stderr,
                 "TypeError: unsupported operand type(s) for //: '%s' and '%s'",
                 self_name->buffer(), other_name->buffer());
    std::exit(1);
  }

  return scope.Escape(floor_div(self, other));
}

// python virtual function
Handle<PyObject> PyObject::Mod(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(self) && IsPySmi(other)) {
    auto result =
        PythonMod(PySmi::cast(*self).value(), PySmi::cast(*other).value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().mod);
  return scope.Escape(GetKlass(*self)->vtable().mod(self, other));
}

// python virtual function
Handle<PyObject> PyObject::Len(Handle<PyObject> self) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().len);
  return scope.Escape(GetKlass(*self)->vtable().len(self));
}

// python virtual function
Tagged<PyBoolean> PyObject::Greater(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  return Isolate::ToPyBoolean(GreaterBool(self, other));
}

bool PyObject::GreaterBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return PySmi::cast(*self).value() > PySmi::cast(*other).value();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().greater);
  return GetKlass(*self)->vtable().greater(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::Less(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  return Isolate::ToPyBoolean(LessBool(self, other));
}

bool PyObject::LessBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return PySmi::cast(*self).value() < PySmi::cast(*other).value();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().less);
  return GetKlass(*self)->vtable().less(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  return Isolate::ToPyBoolean(EqualBool(self, other));
}

bool PyObject::EqualBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：直接比较内存地址
  // 特别注意：如果self和other都是smi，这时候handle引用的就是
  // 裸的smi。因此调用is_identical_to也可以完成比较！！！
  if (self.is_identical_to(other)) {
    return true;
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().equal);
  return GetKlass(*self)->vtable().equal(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::NotEqual(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  return Isolate::ToPyBoolean(NotEqualBool(self, other));
}

bool PyObject::NotEqualBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return PySmi::cast(*self).value() != PySmi::cast(*other).value();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().not_equal);
  return GetKlass(*self)->vtable().not_equal(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::GreaterEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  return Isolate::ToPyBoolean(GreaterEqualBool(self, other));
}

bool PyObject::GreaterEqualBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return PySmi::cast(*self).value() >= PySmi::cast(*other).value();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().ge);
  return GetKlass(*self)->vtable().ge(self, other);
}

// python virtual function
Tagged<PyBoolean> PyObject::LessEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  return Isolate::ToPyBoolean(LessEqualBool(self, other));
}

bool PyObject::LessEqualBool(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return PySmi::cast(*self).value() <= PySmi::cast(*other).value();
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable().le);
  return GetKlass(*self)->vtable().le(self, other);
}

// python virtual function
Handle<PyObject> PyObject::GetAttr(Handle<PyObject> self,
                                   Handle<PyObject> attr_name,
                                   bool is_try) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().getattr);
  return scope.Escape(
      GetKlass(*self)->vtable().getattr(self, attr_name, is_try));
}

Handle<PyObject> PyObject::GetAttrForCall(Handle<PyObject> self,
                                          Handle<PyObject> attr_name,
                                          Handle<PyObject>& self_or_null) {
  EscapableHandleScope scope;

  self_or_null = Handle<PyObject>::null();

  Tagged<Klass> klass = GetKlass(*self);
  // Fast Path:
  // 对于一般对象，直接查询对象方法对应的裸的PyFunction对象，绕开临时生成
  // MethodObject对象的操作。
  if (klass->vtable().getattr == &Klass::Virtual_Default_GetAttr) {
    return scope.Escape(
        Klass::Virtual_Default_GetAttrForCall(self, attr_name, self_or_null));
  }

  // Fallback
  Handle<PyObject> value = GetAttr(self, attr_name, false);
  if (IsMethodObject(value)) {
    auto method = Handle<MethodObject>::cast(value);
    self_or_null = method->owner();
    return scope.Escape(method->func());
  }
  return scope.Escape(value);
}

// python virtual function
void PyObject::SetAttr(Handle<PyObject> self,
                       Handle<PyObject> attr_name,
                       Handle<PyObject> attr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().setattr);
  GetKlass(*self)->vtable().setattr(self, attr_name, attr_value);
}

// python virtual function
Handle<PyObject> PyObject::Subscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr_name) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().subscr);
  return scope.Escape(GetKlass(*self)->vtable().subscr(self, subscr_name));
}

// python virtual function
void PyObject::StoreSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name,
                           Handle<PyObject> subscr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().store_subscr);
  GetKlass(*self)->vtable().store_subscr(self, subscr_name, subscr_value);
}

// python virtual function
Tagged<PyBoolean> PyObject::Contains(Handle<PyObject> self,
                                     Handle<PyObject> target) {
  return Isolate::ToPyBoolean(ContainsBool(self, target));
}

bool PyObject::ContainsBool(Handle<PyObject> self, Handle<PyObject> target) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().contains);
  return GetKlass(*self)->vtable().contains(self, target);
}

// python virtual function
Handle<PyObject> PyObject::Iter(Handle<PyObject> self) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().iter);
  return scope.Escape(GetKlass(*self)->vtable().iter(self));
}

// python virtual function
Handle<PyObject> PyObject::Next(Handle<PyObject> self) {
  EscapableHandleScope scope;

  assert(GetKlass(*self)->vtable().next);
  return scope.Escape(GetKlass(*self)->vtable().next(self));
}

// python virtual function
void PyObject::DeletSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().del_subscr);
  GetKlass(*self)->vtable().del_subscr(self, subscr_name);
}

// python virtual function
uint64_t PyObject::Hash(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable().hash);
  return GetKlass(*self)->vtable().hash(self);
}

Handle<PyObject> PyObject::Call(Handle<PyObject> self,
                                Handle<PyObject> host,
                                Handle<PyObject> args,
                                Handle<PyObject> kwargs) {
  EscapableHandleScope scope;

  auto* call_method = GetKlass(*self)->vtable().call;
  return scope.Escape(call_method(self, host, args, kwargs));
}

// python virtual function
size_t PyObject::GetInstanceSize(Tagged<PyObject> self) {
  return GetKlass(self)->vtable().instance_size(self);
}

// python virtual function
void PyObject::Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  v->VisitPointer(&self->properties_);

  assert(GetKlass(self)->vtable().iterate);
  GetKlass(self)->vtable().iterate(self, v);
}

}  // namespace saauso::internal
