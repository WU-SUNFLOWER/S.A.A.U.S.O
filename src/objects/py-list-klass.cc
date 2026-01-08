// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-klass.h"

#include <cstdio>

#include "src/code/pyc-file-parser.h"
#include "src/heap/heap.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

PyListKlass* PyListKlass::instance_ = nullptr;

// static
PyListKlass* PyListKlass::GetInstance() {
  if (instance_ == nullptr) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::Initialize() {
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
  vtable_.instance_size = &Virtual_InstanceSize;

  // 设置类名
  set_name(PyString::NewInstance("list"));
}

Handle<PyObject> PyListKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(PySmi::FromInt(Handle<PyList>::Cast(self)->length()));
}

void PyListKlass::Virtual_Print(Handle<PyObject> self) {
  auto list = Handle<PyList>::Cast(self);

  std::printf("[");

  if (list->length() > 0) {
    list->Get(0)->Print();
  }

  for (auto i = 1; i < list->length(); ++i) {
    std::printf(", ");
    list->Get(i)->Print();
  }

  std::printf("]");
}

Handle<PyObject> PyListKlass::Virtual_Add(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  auto list1 = Handle<PyList>::Cast(self);
  auto list2 = Handle<PyList>::Cast(other);

  auto new_result = PyList::NewInstance(list1->length() + list2->length());
  for (auto i = 0; i < list1->length(); ++i) {
    new_result->Append(list1->Get(i));
  }

  for (auto i = 0; i < list2->length(); ++i) {
    new_result->Append(list2->Get(i));
  }

  return new_result;
}

Handle<PyObject> PyListKlass::Virtual_Mul(Handle<PyObject> self,
                                          Handle<PyObject> coeff) {
  auto list = Handle<PyList>::Cast(self);
  auto decoded_coeff =
      std::max(static_cast<int64_t>(0), Handle<PySmi>::Cast(coeff)->value());

  auto result = PyList::NewInstance(list->length() * decoded_coeff);
  while (decoded_coeff-- > 0) {
    for (int i = 0; i < list->length(); ++i) {
      result->Append(list->Get(i));
    }
  }

  return result;
}

Handle<PyObject> PyListKlass::Virtual_Subscr(Handle<PyObject> self,
                                             Handle<PyObject> subscr) {
  auto decoded_subscr = Handle<PySmi>::Cast(subscr)->value();
  return Handle<PyList>::Cast(self)->Get(decoded_subscr);
}

void PyListKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                      Handle<PyObject> subscr,
                                      Handle<PyObject> value) {
  auto list = Handle<PyList>::Cast(self);

  auto decoded_subscr = Handle<PySmi>::Cast(subscr)->value();
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::printf("IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->Set(decoded_subscr, value);
}

void PyListKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                    Handle<PyObject> subscr) {
  auto list = Handle<PyList>::Cast(self);

  auto decoded_subscr = Handle<PySmi>::Cast(subscr)->value();
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    std::printf("IndexError: list assignment index out of range");
    std::exit(1);
  }

  list->Remove(decoded_subscr);
}

Tagged<PyBoolean> PyListKlass::Virtual_Less(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  auto list_l = Handle<PyList>::Cast(self);
  auto list_r = Handle<PyList>::Cast(other);
  auto min_len = std::min(list_l->length(), list_r->length());

  for (auto i = 0; i < min_len; ++i) {
    auto l = list_l->Get(i);
    auto r = list_r->Get(i);
    if (l->Less(r)) {
      return Universe::py_true_object_;
    }
    if (l->Greater(r)) {
      return Universe::py_false_object_;
    }
  }

  return Universe::ToPyBoolean(list_l->length() < list_r->length());
}

Handle<PyObject> PyListKlass::Virtual_Iter(Handle<PyObject> self) {
  // TODO: 生成迭代器
  return Handle<PyObject>::Null();
}

Tagged<PyBoolean> PyListKlass::Virtual_Contains(Handle<PyObject> self,
                                                Handle<PyObject> target) {
  auto list = Handle<PyList>::Cast(self);
  for (auto i = 0; i < list->length(); ++i) {
    if (list->Get(i)->Equal(target)) {
      return Universe::py_true_object_;
    }
  }

  return Universe::py_false_object_;
}

size_t PyListKlass::Virtual_InstanceSize(PyObject* self) {
  return sizeof(PyList);
}

}  // namespace saauso::internal
