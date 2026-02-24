// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs-klass.h"

#include <cstdio>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
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
#include "src/utils/maybe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
Tagged<PyBooleanKlass> PyBooleanKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyBooleanKlass> instance = isolate->py_boolean_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_boolean_klass(instance);
  }
  return instance;
}

void PyBooleanKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

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
  Isolate::Current()->set_py_boolean_klass(Tagged<PyBooleanKlass>::null());
}

// static
MaybeHandle<PyObject> PyBooleanKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf(Handle<PyBoolean>::cast(self)->value() ? "True" : "False");
  return Handle<PyObject>(
      Tagged<PyObject>::cast(Isolate::Current()->py_none_object()));
}

// static
Maybe<bool> PyBooleanKlass::Virtual_Equal(Handle<PyObject> self,
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

  return Maybe<bool>(result);
}

// static
Maybe<bool> PyBooleanKlass::Virtual_NotEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  Maybe<bool> equal = Virtual_Equal(self, other);
  if (equal.IsNothing()) {
    return kNullMaybe;
  }
  return Maybe<bool>(!equal.ToChecked());
}

// static
Maybe<uint64_t> PyBooleanKlass::Virtual_Hash(Handle<PyObject> self) {
  return Maybe<uint64_t>(Handle<PyBoolean>::cast(self)->value());
}

///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
Tagged<PyNoneKlass> PyNoneKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyNoneKlass> instance = isolate->py_none_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_none_klass(instance);
  }
  return instance;
}

void PyNoneKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

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
  Isolate::Current()->set_py_none_klass(Tagged<PyNoneKlass>::null());
}

// static
MaybeHandle<PyObject> PyNoneKlass::Virtual_Print(Handle<PyObject> self) {
  (void)self;
  std::printf("None");
  return Handle<PyObject>(
      Tagged<PyObject>::cast(Isolate::Current()->py_none_object()));
}

// static
Maybe<bool> PyNoneKlass::Virtual_Equal(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  assert(IsPyNone(self));
  return Maybe<bool>(self.is_identical_to(other));
}

// static
Maybe<bool> PyNoneKlass::Virtual_NotEqual(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  Maybe<bool> equal = Virtual_Equal(self, other);
  if (equal.IsNothing()) {
    return kNullMaybe;
  }
  return Maybe<bool>(!equal.ToChecked());
}

// static
Maybe<uint64_t> PyNoneKlass::Virtual_Hash(Handle<PyObject> self) {
  assert(IsPyNone(self));
  return Maybe<uint64_t>((*self).ptr());  // None使用自己的地址作为哈希值
}

}  // namespace saauso::internal
