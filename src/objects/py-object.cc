// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object.h"

#include <cassert>

#include "py-object.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/objects/klass.h"
#include "src/objects/mark-word.h"
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

// python virtual function
void PyObject::Print(Handle<PyObject> self) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.print);
  GetKlass(*self)->vtable_.print(self);
}

// python virtual function
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

// python virtual function
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

// python virtual function
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
        PythonMod(PySmi::Cast(*self).value(), PySmi::Cast(*other).value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  HandleScope scope;

  assert(GetKlass(*self)->vtable_.mod);
  return GetKlass(*self)->vtable_.mod(self, other).EscapeFrom(&scope);
}

// python virtual function
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

// python virtual function
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

// python virtual function
Tagged<PyBoolean> PyObject::Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi(*self) && IsPySmi(*other)) {
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() ==
                                 PySmi::Cast(*other).value());
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
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() !=
                                 PySmi::Cast(*other).value());
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
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() >=
                                 PySmi::Cast(*other).value());
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
    return Universe::ToPyBoolean(PySmi::Cast(*self).value() <=
                                 PySmi::Cast(*other).value());
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
Handle<PyObject> PyObject::SetAttr(Handle<PyObject> self,
                                   Handle<PyObject> attr_name,
                                   Handle<PyObject> attr_value) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.setattr);
  return GetKlass(*self)
      ->vtable_.setattr(self, attr_name, attr_value)
      .EscapeFrom(&scope);
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
size_t PyObject::GetInstanceSize(Handle<PyObject> self) {
  return GetInstanceSize(*self);
}

// python virtual function
size_t PyObject::GetInstanceSize(Tagged<PyObject> self) {
  return GetKlass(self)->vtable_.instance_size(self);
}

// python virtual function
void PyObject::Iterate(Handle<PyObject> self, ObjectVisitor* v) {
  HandleScope scope;

  assert(GetKlass(*self)->vtable_.iterate);
  GetKlass(*self)->vtable_.iterate(self, v);
}

}  // namespace saauso::internal
