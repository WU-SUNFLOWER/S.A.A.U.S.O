// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#include <cassert>

#include "py-object.h"
#include "src/handles/handles.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Klass* PyObject::GetKlass(Tagged<PyObject> object) {
  // 特化：Smi使用PySmiKlass，使得它表现得像一个标准的Python对象
  if (IsPySmi(object)) {
    return PySmiKlass::GetInstance();
  }

  assert(IsHeapObject(object));
  assert(object->klass_ != nullptr);

  return object->klass_;
}

void PyObject::SetKlass(Tagged<PyObject> object, Klass* klass) {
  assert(klass != nullptr);
  assert(IsHeapObject(object));
  object->klass_ = klass;
}

///////////////////////////////////////////////////////////////////
// 类型判断工具函数 开始

#define IMPL_PY_CHECKER_WITH_HANDLE_ARG(name) \
  bool Is##name(Handle<PyObject> object) {    \
    return Is##name(*object);                 \
  }

#define IMPL_PY_CHECKER_BY_KLASS(name)                               \
  bool Is##name(Tagged<PyObject> object) {                           \
    return PyObject::GetKlass(object) == name##Klass::GetInstance(); \
  }                                                                  \
  IMPL_PY_CHECKER_WITH_HANDLE_ARG(name)

// 实现IsPyFloat、IsPyString、IsPyList等一系列函数
PY_TYPE_IN_HEAP_LIST(IMPL_PY_CHECKER_BY_KLASS)
#undef IMPL_PY_CHECKER_BY_KLASS

bool IsPySmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

bool IsPyTrue(Tagged<PyObject> object) {
  return object.ptr() == Tagged<PyObject>(Universe::py_true_object_).ptr();
}

bool IsPyFalse(Tagged<PyObject> object) {
  return object.ptr() == Tagged<PyObject>(Universe::py_false_object_).ptr();
}

bool IsHeapObject(Tagged<PyObject> object) {
  return !IsPySmi(object);
}

bool IsGcAbleObject(Tagged<PyObject> object) {
  return !IsPySmi(object) && !IsPyBoolean(object) && !IsPyNone(object);
}

IMPL_PY_CHECKER_WITH_HANDLE_ARG(PySmi)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTrue)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFalse)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(GcAbleObject)
#undef IMPL_PY_CHECKER_WITH_HANDLE_ARG

///////////////////////////////////////////////////////////////////
// 多态虚方法入口 开始

void PyObject::Print(Handle<PyObject> self) {
  HandleScope scope;

  auto* klass = PyObject::GetKlass(*self);
  assert(klass);

  klass->vtable_.print(self);
}

Handle<PyObject> PyObject::Add(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*self)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(*self).value() +
                                           PySmi::Cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.add);
  return GetKlass(*self)->vtable_.add(self, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Sub(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(*self).value() -
                                           PySmi::Cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.sub);
  return GetKlass(*self)->vtable_.sub(self, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Mul(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(*self).value() *
                                           PySmi::Cast(*other).value()));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mul);
  return GetKlass(*self)->vtable_.mul(self, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Div(Handle<PyObject> self, Handle<PyObject> other) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.div);
  return GetKlass(*self)->vtable_.div(self, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Mod(Handle<PyObject> self, Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    auto result =
        PythonMod(PySmi::Cast(*self).value(), PySmi::Cast(*other).value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mod);
  return GetKlass(*self)->vtable_.mod(self, other).EscapeFrom(&scope);
}

Tagged<PyBoolean> PyObject::Greater(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() >
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.greater);
  return GetKlass(*self)->vtable_.greater(self, other);
}

Tagged<PyBoolean> PyObject::Less(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() <
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.less);
  return GetKlass(*self)->vtable_.less(self, other);
}

Tagged<PyBoolean> PyObject::Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() ==
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mod);
  return GetKlass(*self)->vtable_.equal(self, other);
}

Tagged<PyBoolean> PyObject::NotEqual(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() !=
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.not_equal);
  return GetKlass(*self)->vtable_.not_equal(self, other);
}

Tagged<PyBoolean> PyObject::GreaterEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() >=
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.ge);
  return GetKlass(*self)->vtable_.ge(self, other);
}

Tagged<PyBoolean> PyObject::LessEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() <=
                                 PySmi::Cast(*other).value());
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.le);
  return GetKlass(*self)->vtable_.le(self, other);
}

Handle<PyObject> PyObject::GetAttr(Handle<PyObject> self,
                                   Handle<PyObject> attr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.getattr);
  return GetKlass(*self)->vtable_.getattr(self, attr_name).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::SetAttr(Handle<PyObject> self,
                                   Handle<PyObject> attr_name,
                                   Handle<PyObject> attr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.setattr);
  return GetKlass(*self)
      ->vtable_.setattr(self, attr_name, attr_value)
      .EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Subscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.subscr);
  return GetKlass(*self)->vtable_.subscr(self, subscr_name).EscapeFrom(&scope);
}

void PyObject::StoreSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name,
                           Handle<PyObject> subscr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.store_subscr);
  GetKlass(*self)->vtable_.store_subscr(self, subscr_name, subscr_value);
}

Tagged<PyBoolean> PyObject::Contains(Handle<PyObject> self,
                                     Handle<PyObject> target) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.contains);
  return GetKlass(*self)->vtable_.contains(self, target);
}

Handle<PyObject> PyObject::Iter(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.iter);
  return GetKlass(*self)->vtable_.iter(self).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Next(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.next);
  return GetKlass(*self)->vtable_.next(self).EscapeFrom(&scope);
}

void PyObject::DeletSubscr(Handle<PyObject> self,
                           Handle<PyObject> subscr_name) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.del_subscr);
  GetKlass(*self)->vtable_.del_subscr(self, subscr_name);
}

size_t PyObject::GetInstanceSize(Handle<PyObject> self) {
  assert(GetKlass(*self)->vtable_.instance_size);
  return GetKlass(*self)->vtable_.instance_size(*self);
}

}  // namespace saauso::internal
