// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-klass.h"

#include <cstdio>
#include <cstdlib>

#include "include/saauso-internal.h"
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
#include "src/runtime/isolate.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NativeMethod_SetDefault(Handle<PyObject> self,
                                         Handle<PyTuple> args,
                                         Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);

  auto value = dict->Get(key);
  // 如果dict中已经有目标key，直接返回对应的value
  if (!value.is_null()) {
    return scope.Escape(value);
  }

  // 确定默认值。
  // 如果args中没有有效的value值，就取None进行填充。
  value = handle(Isolate::Current()->py_none_object());
  if (args->length() > 1) {
    value = args->Get(1);
  }

  // 填充默认值
  PyDict::Put(dict, key, value);

  return scope.Escape(value);
}

Handle<PyObject> NativeMethod_Pop(Handle<PyObject> self,
                                  Handle<PyTuple> args,
                                  Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  if (args->length() < 1 || args->length() > 2) {
    std::fprintf(stderr,
                 "TypeError: pop expected at most 2 arguments, got %lld\n",
                 static_cast<long long>(args->length()));
    std::exit(1);
  }

  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);
  auto value = dict->Get(key);
  if (!value.is_null()) {
    dict->Remove(key);
    return scope.Escape(value);
  }

  if (args->length() == 2) {
    return scope.Escape(args->Get(1));
  }

  std::printf("KeyError: ");
  PyObject::Print(key);
  std::printf("\n");
  std::exit(1);
}

Handle<PyObject> NativeMethod_Keys(Handle<PyObject> self,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictKeys::NewInstance(self));
}

Handle<PyObject> NativeMethod_Values(Handle<PyObject> self,
                                     Handle<PyTuple> args,
                                     Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictValues::NewInstance(self));
}

Handle<PyObject> NativeMethod_Items(Handle<PyObject> self,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  return scope.Escape(PyDictItems::NewInstance(self));
}

Handle<PyObject> NativeMethod_Get(Handle<PyObject> self,
                                  Handle<PyTuple> args,
                                  Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto dict = Handle<PyDict>::cast(self);
  auto key = args->Get(0);

  Handle<PyObject> result = dict->Get(key);
  if (result.is_null()) {
    result = handle(Isolate::Current()->py_none_object());
  }

  return scope.Escape(result);
}

}  // namespace

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
  vtable_.construct_instance = &Virtual_Default_ConstructInstance;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictKlass::Initialize() {
  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("setdefault");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_SetDefault, prop_name));

  prop_name = PyString::NewInstance("pop");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Pop, prop_name));

  prop_name = PyString::NewInstance("keys");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Keys, prop_name));

  prop_name = PyString::NewInstance("values");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Values, prop_name));

  prop_name = PyString::NewInstance("items");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Items, prop_name));

  prop_name = PyString::NewInstance("get");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Get, prop_name));

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
  if (!dict->Contains(subscr)) {
    std::printf("KeyError: ");
    PyObject::Print(subscr);
    std::printf("\n");
    std::exit(1);
  }
  dict->Remove(subscr);
}

// static
bool PyDictKlass::Virtual_Contains(Handle<PyObject> self,
                                   Handle<PyObject> subscr) {
  return Handle<PyDict>::cast(self)->Contains(subscr);
}

Handle<PyObject> PyDictKlass::Virtual_Default_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyDictKlass::GetInstance());

  // TODO
  return PyDict::NewInstance();
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
