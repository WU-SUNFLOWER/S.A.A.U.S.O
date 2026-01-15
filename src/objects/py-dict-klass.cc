// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-klass.h"

#include <cstdio>
#include <cstdlib>

#include "include/saauso-internal.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NativeMethod_SetDefault(Handle<PyList> args,
                                         Handle<PyDict> kwargs) {
  auto object = Handle<PyDict>::cast(args->Get(0));
  auto key = args->Get(1);
  auto value = args->Get(2);

  if (IsPyFalse(object->Contains(key))) {
    PyDict::Put(object, key, value);
  }

  return handle(Universe::py_none_object_);
}

}  // namespace

////////////////////////////////////////////////////////////////////

Tagged<PyDictKlass> PyDictKlass::instance_(kNullAddress);

// static
Tagged<PyDictKlass> PyDictKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyDictKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyDictKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DeleteSubscr;
  vtable_.contains = &Virtual_Contains;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictKlass::Initialize() {
  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("setdefault");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_SetDefault, prop_name));

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
    if (!key.IsNull()) {
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
Handle<PyObject> PyDictKlass::Virtual_Len(Handle<PyObject> self) {
  auto value = Handle<PyDict>::cast(self)->occupied();
  return Handle<PyObject>(PySmi::FromInt(value));
}

// static
Tagged<PyBoolean> PyDictKlass::Virtual_Equal(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  assert(IsPyDict(self));

  // fast path
  if (self.is_identical_to(other)) {
    return Universe::py_true_object_;
  }

  if (!IsPyDict(other)) {
    return Universe::py_false_object_;
  }

  auto d1 = Handle<PyDict>::cast(self);
  auto d2 = Handle<PyDict>::cast(other);

  if (d1->occupied() != d2->occupied()) {
    return Universe::py_false_object_;
  }

  for (int64_t i = 0; i < d1->capacity(); ++i) {
    auto k1 = d1->data()->Get(i << 1);
    if (!k1.IsNull()) {
      auto v1 = d1->data()->Get((i << 1) + 1);
      auto v2_handle = d2->Get(Handle<PyObject>(k1));
      if (v2_handle.IsNull()) {
        return Universe::py_false_object_;
      }

      if (IsPyFalse(PyObject::Equal(Handle<PyObject>(v1), v2_handle))) {
        return Universe::py_false_object_;
      }
    }
  }
  return Universe::py_true_object_;
}

// static
Tagged<PyBoolean> PyDictKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  if (IsPyTrue(Virtual_Equal(self, other))) {
    return Universe::py_false_object_;
  }
  return Universe::py_true_object_;
}

// static
Handle<PyObject> PyDictKlass::Virtual_Subscr(Handle<PyObject> self,
                                             Handle<PyObject> subscr) {
  auto result = Handle<PyDict>::cast(self)->Get(subscr);
  if (result.IsNull()) {
    std::printf("KeyError: ");
    PyObject::Print(subscr);
    std::printf("\n");
    std::exit(1);
  }
  return result;
}

// static
void PyDictKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr,
                                      Handle<PyObject> value) {
  PyDict::Put(self, subscr, value);
}

// static
void PyDictKlass::Virtual_DeleteSubscr(Handle<PyObject> self,
                                       Handle<PyObject> subscr) {
  auto dict = Handle<PyDict>::cast(self);
  if (IsPyFalse(dict->Contains(subscr))) {
    std::printf("KeyError: ");
    PyObject::Print(subscr);
    std::printf("\n");
    std::exit(1);
  }
  dict->Remove(subscr);
}

// static
Tagged<PyBoolean> PyDictKlass::Virtual_Contains(Handle<PyObject> self,
                                                Handle<PyObject> subscr) {
  return Handle<PyDict>::cast(self)->Contains(subscr);
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
  instance_ = Tagged<PyDictKlass>::Null();
}

}  // namespace saauso::internal
