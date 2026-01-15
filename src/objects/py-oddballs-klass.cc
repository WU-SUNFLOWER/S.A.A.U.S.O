// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs-klass.h"

#include <cstdio>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PyBooleanKlass> PyBooleanKlass::instance_(nullptr);
Tagged<PyNoneKlass> PyNoneKlass::instance_(nullptr);

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
Tagged<PyBooleanKlass> PyBooleanKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyBooleanKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.hash = &Virtual_Hash;
}

void PyBooleanKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化klass_properties
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PySmiKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("bool"));
}

// static
void PyBooleanKlass::Finalize() {
  instance_ = Tagged<PyBooleanKlass>::Null();
}

// static
void PyBooleanKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf(Handle<PyBoolean>::cast(self)->value() ? "True" : "False");
}

// static
Tagged<PyBoolean> PyBooleanKlass::Virtual_Equal(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  bool v = Handle<PyBoolean>::cast(self)->value();

  bool result = false;
  if (IsPySmi(other)) {
    result = PySmi::ToInt(Handle<PySmi>::cast(other)) == v;
  } else if (IsPyFloat(other)) {
    result = Handle<PyFloat>::cast(other)->value() == v;
  } else if (IsPyBoolean(other)) {
    result = Handle<PyBoolean>::cast(other)->value() == v;
  }

  return Universe::ToPyBoolean(result);
}

// static
Tagged<PyBoolean> PyBooleanKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                   Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

// static
uint64_t PyBooleanKlass::Virtual_Hash(Handle<PyObject> self) {
  return Handle<PyBoolean>::cast(self)->value();
}

///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
Tagged<PyNoneKlass> PyNoneKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyNoneKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.hash = &Virtual_Hash;
}

void PyNoneKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("NoneType"));
}

void PyNoneKlass::Finalize() {
  instance_ = Tagged<PyNoneKlass>::Null();
}

// static
void PyNoneKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("None");
}

// static
Tagged<PyBoolean> PyNoneKlass::Virtual_Equal(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(IsPyNone(self));
  return Universe::ToPyBoolean((*self).ptr() == (*other).ptr());
}

// static
Tagged<PyBoolean> PyNoneKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  return Virtual_Equal(self, other)->Reverse();
}

// static
uint64_t PyNoneKlass::Virtual_Hash(Handle<PyObject> self) {
  assert(IsPyNone(self));
  return (*self).ptr();  // None使用自己的地址作为哈希值
}

}  // namespace saauso::internal
