// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-klass.h"

#include <cstdio>
#include <cstdlib>

#include "include/saauso-internal.h"
#include "src/builtins/builtins-py-dict-methods.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-dict.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static
Tagged<PyDictKlass> PyDictKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictKlass> instance = isolate->py_dict_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_klass(instance);
  }
  return instance;
}

void PyDictKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DeleteSubscr;
  vtable_.contains = &Virtual_Contains;
  vtable_.iter = &Virtual_Iter;
  vtable_.construct_instance = &Virtual_ConstructInstance;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictKlass::Initialize() {
  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();

  // 安装内建方法
  PyDictBuiltinMethods::Install(klass_properties);

  set_klass_properties(klass_properties);

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("dict"));
}

// static
void PyDictKlass::Virtual_Print(Handle<PyObject> self) {
  auto dict = Handle<PyDict>::cast(self);
  std::printf("{");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    auto key = dict->data()->Get(i << 1);
    if (!key.is_null()) {
      if (!first) {
        std::printf(", ");
      }
      first = false;
      PyObject::Print(Handle<PyObject>(key));
      std::printf(": ");
      PyObject::Print(Handle<PyObject>(dict->data()->Get((i << 1) + 1)));
    }
  }
  std::printf("}");
}

// static
Handle<PyObject> PyDictKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyDictKeyIterator::NewInstance(self);
}

// static
Handle<PyObject> PyDictKlass::Virtual_Len(Handle<PyObject> self) {
  auto value = Handle<PyDict>::cast(self)->occupied();
  return Handle<PyObject>(PySmi::FromInt(value));
}

// static
bool PyDictKlass::Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other) {
  assert(IsPyDict(self));

  // fast path
  if (self.is_identical_to(other)) {
    return true;
  }

  if (!IsPyDict(other)) {
    return false;
  }

  auto d1 = Handle<PyDict>::cast(self);
  auto d2 = Handle<PyDict>::cast(other);

  if (d1->occupied() != d2->occupied()) {
    return false;
  }

  for (int64_t i = 0; i < d1->capacity(); ++i) {
    auto k1 = d1->data()->Get(i << 1);
    if (!k1.is_null()) {
      auto v1 = d1->data()->Get((i << 1) + 1);
      auto v2_handle = d2->Get(Handle<PyObject>(k1));
      if (v2_handle.is_null()) {
        return false;
      }

      if (!PyObject::EqualBool(Handle<PyObject>(v1), v2_handle)) {
        return false;
      }
    }
  }
  return true;
}

// static
bool PyDictKlass::Virtual_NotEqual(Handle<PyObject> self,
                                   Handle<PyObject> other) {
  return !Virtual_Equal(self, other);
}

// static
Handle<PyObject> PyDictKlass::Virtual_Subscr(Handle<PyObject> self,
                                             Handle<PyObject> subscr) {
  auto result = Handle<PyDict>::cast(self)->Get(subscr);
  if (result.is_null()) {
    // TODO: Repr机制完善后，支持把key打印出来
    Runtime_ThrowError(ExceptionType::kKeyError, "key not found in dict");
    return Handle<PyObject>::null();
  }
  return result;
}

// static
void PyDictKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr,
                                      Handle<PyObject> value) {
  PyDict::Put(Handle<PyDict>::cast(self), subscr, value);
}

// static
void PyDictKlass::Virtual_DeleteSubscr(Handle<PyObject> self,
                                       Handle<PyObject> subscr) {
  auto dict = Handle<PyDict>::cast(self);
  if (!dict->Contains(subscr)) {
    // TODO: Repr机制完善后，支持把key打印出来
    Runtime_ThrowError(ExceptionType::kKeyError, "key not found in dict");
    return;
  }
  dict->Remove(subscr);
}

// static
bool PyDictKlass::Virtual_Contains(Handle<PyObject> self,
                                   Handle<PyObject> subscr) {
  return Handle<PyDict>::cast(self)->Contains(subscr);
}

Handle<PyObject> PyDictKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyDictKlass::GetInstance());
  Handle<PyObject> result;
  if (!Runtime_NewDict(args, kwargs).ToHandle(&result)) {
    return Handle<PyObject>::null();
  }
  return result;
}

// static
size_t PyDictKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyDict);
}

// static
void PyDictKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  auto dict = Tagged<PyDict>::cast(self);
  v->VisitPointer(&dict->data_);
}

// static
void PyDictKlass::Finalize() {
  Isolate::Current()->set_py_dict_klass(Tagged<PyDictKlass>::null());
}

}  // namespace saauso::internal
