// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-views-klass.h"

#include <cassert>
#include <cstdio>

#include "src/builtins/builtins-py-dict-views-methods.h"
#include "src/execution/exception-types.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

template <typename IteratorType, typename Getter>
Handle<PyObject> NextFromIterator(Handle<PyObject> self, Getter getter) {
  EscapableHandleScope scope;

  auto iterator = Handle<IteratorType>::cast(self);
  auto dict = iterator->owner();
  assert(!dict.is_null());

  int64_t index = iterator->iter_index();
  for (; index < dict->capacity(); ++index) {
    Handle<PyObject> result = getter(dict, index);
    if (result.is_null()) {
      continue;
    }
    iterator->set_iter_index(index + 1);
    return scope.Escape(result);
  }

  iterator->set_iter_index(dict->capacity());
  return Handle<PyObject>::null();
}

template <typename ViewType>
Handle<PyObject> DictViewLen(Handle<PyObject> self) {
  auto dict = Handle<ViewType>::cast(self)->owner();
  return Handle<PyObject>(PySmi::FromInt(dict->occupied()));
}

}  // namespace

Tagged<PyDictKeysKlass> PyDictKeysKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictKeysKlass> instance = isolate->py_dict_keys_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictKeysKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_keys_klass(instance);
  }
  return instance;
}

void PyDictKeysKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.len_ = &Virtual_Len;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictKeysKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::NewInstance("dict_keys"));

  return JustVoid();
}

void PyDictKeysKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_keys_klass(Tagged<PyDictKeysKlass>::null());
}

MaybeHandle<PyObject> PyDictKeysKlass::Virtual_Iter(Handle<PyObject> self) {
  return Isolate::Current()->factory()->NewPyDictKeyIterator(
      Handle<PyDictKeys>::cast(self)->owner());
}

MaybeHandle<PyObject> PyDictKeysKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictKeys>(self);
}

Maybe<bool> PyDictKeysKlass::Virtual_Contains(Handle<PyObject> self,
                                              Handle<PyObject> subscr) {
  auto dict = Handle<PyDictKeys>::cast(self)->owner();
  return dict->ContainsKey(subscr);
}

size_t PyDictKeysKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictKeys));
}

void PyDictKeysKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyDictKeys(self));
  auto view = Tagged<PyDictKeys>::cast(self);
  v->VisitPointer(&view->owner_);
}

Tagged<PyDictValuesKlass> PyDictValuesKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictValuesKlass> instance = isolate->py_dict_values_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictValuesKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_values_klass(instance);
  }
  return instance;
}

void PyDictValuesKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.len_ = &Virtual_Len;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictValuesKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::NewInstance("dict_values"));

  return JustVoid();
}

void PyDictValuesKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_values_klass(Tagged<PyDictValuesKlass>::null());
}

MaybeHandle<PyObject> PyDictValuesKlass::Virtual_Iter(Handle<PyObject> self) {
  return Isolate::Current()->factory()->NewPyDictValueIterator(
      Handle<PyDictValues>::cast(self)->owner());
}

MaybeHandle<PyObject> PyDictValuesKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictValues>(self);
}

Maybe<bool> PyDictValuesKlass::Virtual_Contains(Handle<PyObject> self,
                                                Handle<PyObject> subscr) {
  HandleScope scope;

  auto dict = Handle<PyDictValues>::cast(self)->owner();
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> value = dict->ValueAtIndex(i);
    if (value.is_null()) {
      continue;
    }

    bool equal;
    ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), equal,
                               PyObject::EqualBool(value, subscr));

    if (equal) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

size_t PyDictValuesKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictValues));
}

void PyDictValuesKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  assert(IsPyDictValues(self));
  auto view = Tagged<PyDictValues>::cast(self);
  v->VisitPointer(&view->owner_);
}

Tagged<PyDictItemsKlass> PyDictItemsKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictItemsKlass> instance = isolate->py_dict_items_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictItemsKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_items_klass(instance);
  }
  return instance;
}

void PyDictItemsKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.len_ = &Virtual_Len;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictItemsKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::NewInstance("dict_items"));

  return JustVoid();
}

void PyDictItemsKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_items_klass(Tagged<PyDictItemsKlass>::null());
}

MaybeHandle<PyObject> PyDictItemsKlass::Virtual_Iter(Handle<PyObject> self) {
  return Isolate::Current()->factory()->NewPyDictItemIterator(
      Handle<PyDictItems>::cast(self)->owner());
}

MaybeHandle<PyObject> PyDictItemsKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictItems>(self);
}

Maybe<bool> PyDictItemsKlass::Virtual_Contains(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  HandleScope scope;
  auto* isolate [[maybe_unused]] = Isolate::Current();

  if (!IsPyTuple(subscr)) {
    return Maybe<bool>(false);
  }

  auto item = Handle<PyTuple>::cast(subscr);
  if (item->length() != 2) {
    return Maybe<bool>(false);
  }

  auto dict = Handle<PyDictItems>::cast(self)->owner();
  auto key = item->Get(0);
  auto value = item->Get(1);

  Handle<PyObject> found_value;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found, dict->Get(key, found_value));

  if (!found) {
    return Maybe<bool>(false);
  }

  bool is_equal = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                             PyObject::EqualBool(found_value, value));
  return Maybe<bool>(is_equal);
}

size_t PyDictItemsKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictItems));
}

void PyDictItemsKlass::Virtual_Iterate(Tagged<PyObject> self,
                                       ObjectVisitor* v) {
  assert(IsPyDictItems(self));
  auto view = Tagged<PyDictItems>::cast(self);
  v->VisitPointer(&view->owner_);
}

Tagged<PyDictKeyIteratorKlass> PyDictKeyIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictKeyIteratorKlass> instance =
      isolate->py_dict_keyiterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictKeyIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_keyiterator_klass(instance);
  }
  return instance;
}

void PyDictKeyIteratorKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.next_ = &Virtual_Next;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictKeyIteratorKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate, PyDictKeyIteratorBuiltinMethods::Install(
                                   isolate, klass_properties));

  set_name(PyString::NewInstance("dict_keyiterator"));

  return JustVoid();
}

void PyDictKeyIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_keyiterator_klass(
      Tagged<PyDictKeyIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictKeyIteratorKlass::Virtual_Iter(
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictKeyIteratorKlass::Virtual_Next(
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictKeyIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->KeyAtIndex(index);
      });
  if (result.is_null()) {
    Runtime_ThrowError(ExceptionType::kStopIteration);
    return kNullMaybeHandle;
  }
  return result;
}

size_t PyDictKeyIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictKeyIterator));
}

void PyDictKeyIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                             ObjectVisitor* v) {
  assert(IsPyDictKeyIterator(self));
  auto iterator = Tagged<PyDictKeyIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

Tagged<PyDictItemIteratorKlass> PyDictItemIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictItemIteratorKlass> instance =
      isolate->py_dict_itemiterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictItemIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_itemiterator_klass(instance);
  }
  return instance;
}

void PyDictItemIteratorKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.next_ = &Virtual_Next;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictItemIteratorKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate, PyDictItemIteratorBuiltinMethods::Install(
                                   isolate, klass_properties));

  set_name(PyString::NewInstance("dict_itemiterator"));

  return JustVoid();
}

void PyDictItemIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_itemiterator_klass(
      Tagged<PyDictItemIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictItemIteratorKlass::Virtual_Iter(
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictItemIteratorKlass::Virtual_Next(
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictItemIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->ItemAtIndex(index);
      });
  if (result.is_null()) {
    Runtime_ThrowError(ExceptionType::kStopIteration);
    return kNullMaybeHandle;
  }
  return result;
}

size_t PyDictItemIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictItemIterator));
}

void PyDictItemIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                              ObjectVisitor* v) {
  assert(IsPyDictItemIterator(self));
  auto iterator = Tagged<PyDictItemIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

Tagged<PyDictValueIteratorKlass> PyDictValueIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictValueIteratorKlass> instance =
      isolate->py_dict_valueiterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictValueIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_valueiterator_klass(instance);
  }
  return instance;
}

void PyDictValueIteratorKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));
  
  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.next_ = &Virtual_Next;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictValueIteratorKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate, PyDictValueIteratorBuiltinMethods::Install(
                                   isolate, klass_properties));

  set_name(PyString::NewInstance("dict_valueiterator"));

  return JustVoid();
}

void PyDictValueIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_valueiterator_klass(
      Tagged<PyDictValueIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictValueIteratorKlass::Virtual_Iter(
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictValueIteratorKlass::Virtual_Next(
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictValueIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->ValueAtIndex(index);
      });
  if (result.is_null()) {
    Runtime_ThrowError(ExceptionType::kStopIteration);
    return kNullMaybeHandle;
  }
  return result;
}

size_t PyDictValueIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictValueIterator));
}

void PyDictValueIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                               ObjectVisitor* v) {
  assert(IsPyDictValueIterator(self));
  auto iterator = Tagged<PyDictValueIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

}  // namespace saauso::internal
