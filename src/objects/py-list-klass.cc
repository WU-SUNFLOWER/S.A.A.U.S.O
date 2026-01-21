// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-klass.h"

#include <cstdio>

#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
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
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NativeMethod_Append(Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  auto object = Handle<PyList>::cast(args->Get(0));
  PyList::Append(object, args->Get(1));

  return handle(Isolate::Current()->py_none_object());
}

}  // namespace

////////////////////////////////////////////////////////////////////

// static
Tagged<PyListKlass> PyListKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyListKlass> instance = isolate->py_list_klass();
  if (instance.IsNull()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(this);

  // 初始化虚函数表
  vtable_.len = &Virtual_Len;
  vtable_.print = &Virtual_Print;
  vtable_.add = &Virtual_Add;
  vtable_.mul = &Virtual_Mul;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DelSubscr;
  vtable_.less = &Virtual_Less;
  vtable_.iter = &Virtual_Iter;
  vtable_.contains = &Virtual_Contains;
  vtable_.equal = &Virtual_Equal;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyListKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置类属性
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("append");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Append, prop_name));

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("list"));
}

void PyListKlass::Finalize() {
  Isolate::Current()->set_py_list_klass(Tagged<PyListKlass>::Null());
}

Handle<PyObject> PyListKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(PySmi::FromInt(Handle<PyList>::cast(self)->length()));
}

void PyListKlass::Virtual_Print(Handle<PyObject> self) {
  auto list = Handle<PyList>::cast(self);

  std::printf("[");

  if (list->length() > 0) {
    PyObject::Print(list->Get(0));
  }

  for (auto i = 1; i < list->length(); ++i) {
    std::printf(", ");
    PyObject::Print(list->Get(i));
  }

  std::printf("]");
}

Handle<PyObject> PyListKlass::Virtual_Add(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  auto list1 = Handle<PyList>::cast(self);
  auto list2 = Handle<PyList>::cast(other);

  auto new_result = PyList::NewInstance(list1->length() + list2->length());
  for (auto i = 0; i < list1->length(); ++i) {
    PyList::Append(new_result, list1->Get(i));
  }

  for (auto i = 0; i < list2->length(); ++i) {
    PyList::Append(new_result, list2->Get(i));
  }

  return new_result;
}

Handle<PyObject> PyListKlass::Virtual_Mul(Handle<PyObject> self,
                                          Handle<PyObject> coeff) {
  if (!IsPySmi(coeff)) {
    std::printf("can't multiply sequence by non-int of type '%.*s'",
                static_cast<int>(PyObject::GetKlass(coeff)->name()->length()),
                PyObject::GetKlass(coeff)->name()->buffer());
    std::exit(1);
  }

  auto list = Handle<PyList>::cast(self);
  auto decoded_coeff = std::max(static_cast<int64_t>(0),
                                PySmi::ToInt(Handle<PySmi>::cast(coeff)));

  auto result = PyList::NewInstance(list->length() * decoded_coeff);
  while (decoded_coeff-- > 0) {
    for (int i = 0; i < list->length(); ++i) {
      PyList::Append(result, list->Get(i));
    }
  }

  return result;
}

Handle<PyObject> PyListKlass::Virtual_Subscr(Handle<PyObject> self,
                                             Handle<PyObject> subscr) {
  if (!IsPySmi(subscr)) {
    std::printf("list indices must be integers or slices, not %.*s",
                static_cast<int>(PyObject::GetKlass(subscr)->name()->length()),
                PyObject::GetKlass(subscr)->name()->buffer());
    std::exit(1);
  }

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  return Handle<PyList>::cast(self)->Get(decoded_subscr);
}

void PyListKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr,
                                      Handle<PyObject> value) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::printf("IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->Set(decoded_subscr, value);
}

void PyListKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                    Handle<PyObject> subscr) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::printf("IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->RemoveByIndex(decoded_subscr);
}

Tagged<PyBoolean> PyListKlass::Virtual_Less(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  auto list_l = Handle<PyList>::cast(self);
  auto list_r = Handle<PyList>::cast(other);
  auto min_len = std::min(list_l->length(), list_r->length());

  for (auto i = 0; i < min_len; ++i) {
    auto l = list_l->Get(i);
    auto r = list_r->Get(i);

    bool result = IsPyTrue(PyObject::Less(l, r));
    if (result) {
      return Isolate::Current()->py_true_object();
    }

    result = IsPyTrue(PyObject::Greater(l, r));
    if (result) {
      return Isolate::Current()->py_false_object();
    }
  }

  return Isolate::ToPyBoolean(list_l->length() < list_r->length());
}

Handle<PyObject> PyListKlass::Virtual_Iter(Handle<PyObject> self) {
  // TODO: 生成迭代器
  return Handle<PyObject>::Null();
}

Tagged<PyBoolean> PyListKlass::Virtual_Contains(Handle<PyObject> self,
                                                Handle<PyObject> target) {
  auto list = Handle<PyList>::cast(self);
  for (auto i = 0; i < list->length(); ++i) {
    auto result = PyObject::Equal(list->Get(i), target);
    if (IsPyTrue(result)) {
      return Isolate::Current()->py_true_object();
    }
  }

  return Isolate::Current()->py_false_object();
}

Tagged<PyBoolean> PyListKlass::Virtual_Equal(Handle<PyObject> self,
                                             Handle<PyObject> target) {
  auto list1 = Handle<PyList>::cast(self);
  auto list2 = Handle<PyList>::cast(target);

  if (list1->length() != list2->length()) {
    return Isolate::Current()->py_false_object();
  }

  for (auto i = 0; i < list1->length(); ++i) {
    if (IsPyFalse(PyObject::Equal(list1->Get(i), list2->Get(i)))) {
      return Isolate::Current()->py_false_object();
    }
  }

  return Isolate::Current()->py_true_object();
}

size_t PyListKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyList));
}

void PyListKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyList(self));
  v->VisitPointer(&Tagged<PyList>::cast(self)->array_);
}

}  // namespace saauso::internal
