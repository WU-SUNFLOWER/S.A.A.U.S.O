// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-object.h"

#include <cassert>

#include "handles/handles.h"
#include "objects/klass.h"
#include "objects/py-oddballs-klass.h"
#include "objects/py-oddballs.h"
#include "objects/py-float-klass.h"
#include "objects/py-list-klass.h"
#include "objects/py-smi-klass.h"
#include "objects/py-smi.h"
#include "objects/py-string-klass.h"
#include "runtime/universe.h"
#include "utils/utils.h"


namespace saauso::internal {

Klass* PyObject::klass() const {
  // 特化：Smi使用PySmiKlass，使得它表现得像一个标准的Python对象
  if (IsPySmi()) {
    return PySmiKlass::GetInstance();
  }

  assert(IsHeapObject());
  assert(klass_ != nullptr);

  return klass_;
}

void PyObject::set_klass(Klass* klass) {
  assert(klass != nullptr);
  assert(IsHeapObject());
  klass_ = klass;
}

///////////////////////////////////////////////////////////////////
// 类型判断工具函数 开始

bool PyObject::IsPyFloat() const {
  return klass() == PyFloatKlass::GetInstance();
}

bool PyObject::IsPyList() const {
  return klass() == PyListKlass::GetInstance();
}

bool PyObject::IsPyString() const {
  return klass() == PyStringKlass::GetInstance();
}

bool PyObject::IsPyBoolean() const {
  return klass() == PyBooleanKlass::GetInstance();
}

bool PyObject::IsPyTrue() const {
  return this == static_cast<PyObject*>(Universe::py_true_object_);
}

bool PyObject::IsPyFalse() const {
  return this == static_cast<PyObject*>(Universe::py_false_object_);
}

bool PyObject::IsPyNone() const {
  return this == static_cast<PyObject*>(Universe::py_none_object_);
}

bool PyObject::IsPySmi() const {
  return (reinterpret_cast<int64_t>(this) & PySmi::kSmiTagMask) ==
         PySmi::kSmiTag;
}

bool PyObject::IsHeapObject() const {
  return !IsPySmi();
}

bool PyObject::IsGcAbleObject() const {
  return !IsPySmi() && !IsPyTrue() && !IsPyFalse() && !IsPyNone();
}

///////////////////////////////////////////////////////////////////
// 多态虚方法入口 开始

void PyObject::Print() {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.print);
  klass()->vtable_.print(this_handle);
}

Handle<PyObject> PyObject::Add(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(this)->value() +
                                           PySmi::Cast(*other)->value()));
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.add);
  return klass()->vtable_.add(this_handle, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Sub(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(this)->value() -
                                           PySmi::Cast(*other)->value()));
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.sub);
  return klass()->vtable_.sub(this_handle, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Mul(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Handle<PyObject>(PySmi::FromInt(PySmi::Cast(this)->value() *
                                           PySmi::Cast(*other)->value()));
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.mul);
  return klass()->vtable_.mul(this_handle, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Div(Handle<PyObject> other) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.div);
  return klass()->vtable_.div(this_handle, other).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Mod(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    auto result =
        PythonMod(PySmi::Cast(this)->value(), PySmi::Cast(*other)->value());
    return Handle<PyObject>(PySmi::FromInt(result));
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.mod);
  return klass()->vtable_.mod(this_handle, other).EscapeFrom(&scope);
}

PyBoolean* PyObject::Greater(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() >
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.greater);
  return klass()->vtable_.greater(this_handle, other);
}

PyBoolean* PyObject::Less(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() <
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.less);
  return klass()->vtable_.less(this_handle, other);
}

PyBoolean* PyObject::Equal(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() ==
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.mod);
  return klass()->vtable_.equal(this_handle, other);
}

PyBoolean* PyObject::NotEqual(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() !=
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.not_equal);
  return klass()->vtable_.not_equal(this_handle, other);
}

PyBoolean* PyObject::GreaterEqual(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() >=
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.ge);
  return klass()->vtable_.ge(this_handle, other);
}

PyBoolean* PyObject::LessEqual(Handle<PyObject> other) {
  // 内联Fast Path：两个Smi之间操作
  if (IsPySmi() && other->IsPySmi()) {
    return Universe::ToPyBoolean(PySmi::Cast(this)->value() <=
                                 PySmi::Cast(*other)->value());
  }

  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.le);
  return klass()->vtable_.le(this_handle, other);
}

Handle<PyObject> PyObject::GetAttr(Handle<PyObject> attr_name) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.getattr);
  return klass()->vtable_.getattr(this_handle, attr_name).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::SetAttr(Handle<PyObject> attr_name,
                                   Handle<PyObject> attr_value) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.setattr);
  return klass()
      ->vtable_.setattr(this_handle, attr_name, attr_value)
      .EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Subscr(Handle<PyObject> subscr_name) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.subscr);
  return klass()->vtable_.subscr(this_handle, subscr_name).EscapeFrom(&scope);
}

void PyObject::StoreSubscr(Handle<PyObject> subscr_name,
                           Handle<PyObject> subscr_value) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.store_subscr);
  klass()->vtable_.store_subscr(this_handle, subscr_name, subscr_value);
}

PyBoolean* PyObject::Contains(Handle<PyObject> target) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.contains);
  return klass()->vtable_.contains(this_handle, target);
}

Handle<PyObject> PyObject::Iter() {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.iter);
  return klass()->vtable_.iter(this_handle).EscapeFrom(&scope);
}

Handle<PyObject> PyObject::Next() {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.next);
  return klass()->vtable_.next(this_handle).EscapeFrom(&scope);
}

void PyObject::DeletSubscr(Handle<PyObject> subscr_name) {
  HandleScope scope;
  Handle<PyObject> this_handle(this);

  assert(klass()->vtable_.del_subscr);
  klass()->vtable_.del_subscr(this_handle, subscr_name);
}

size_t PyObject::GetInstanceSize() {
  assert(klass()->vtable_.instance_size);
  return klass()->vtable_.instance_size(this);
}

}  // namespace saauso::internal
