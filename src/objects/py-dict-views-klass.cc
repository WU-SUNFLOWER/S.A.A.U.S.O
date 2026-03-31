// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-views-klass.h"

#include <cassert>
#include <cstdio>

#include "include/saauso-maybe.h"
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
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

template <typename IteratorType, typename Getter>
Handle<PyObject> NextFromIterator(Isolate* isolate,
                                  Handle<PyObject> self,
                                  Getter getter) {
  EscapableHandleScope scope(isolate);

  auto iterator = Handle<IteratorType>::cast(self);
  auto dict = iterator->owner(isolate);
  assert(!dict.is_null());

  int64_t index = iterator->iter_index();
  for (; index < dict->capacity(); ++index) {
    Handle<PyObject> result = getter(isolate, dict, index);
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
Handle<PyObject> DictViewLen(Isolate* isolate, Handle<PyObject> self) {
  auto dict = Handle<ViewType>::cast(self)->owner(isolate);
  return isolate->factory()->NewSmiFromInt(dict->occupied());
}

}  // namespace

Tagged<PyDictKeysKlass> PyDictKeysKlass::GetInstance(Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictKeys);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  set_klass_properties(PyDict::New(isolate));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::New(isolate, "dict_keys"));

  return JustVoid();
}

void PyDictKeysKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_keys_klass(Tagged<PyDictKeysKlass>::null());
}

MaybeHandle<PyObject> PyDictKeysKlass::Virtual_Iter(Isolate* isolate,
                                                    Handle<PyObject> self) {
  return isolate->factory()->NewPyDictKeyIterator(
      Handle<PyDictKeys>::cast(self)->owner(isolate));
}

MaybeHandle<PyObject> PyDictKeysKlass::Virtual_Len(Isolate* isolate,
                                                   Handle<PyObject> self) {
  return DictViewLen<PyDictKeys>(isolate, self);
}

Maybe<bool> PyDictKeysKlass::Virtual_Contains(Isolate* isolate,
                                              Handle<PyObject> self,
                                              Handle<PyObject> subscr) {
  auto dict = Handle<PyDictKeys>::cast(self)->owner(isolate);
  return dict->ContainsKey(subscr, isolate);
}

size_t PyDictKeysKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictKeys));
}

void PyDictKeysKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyDictKeys(self));
  auto view = Tagged<PyDictKeys>::cast(self);
  v->VisitPointer(&view->owner_);
}

Tagged<PyDictValuesKlass> PyDictValuesKlass::GetInstance(Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictValues);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  set_klass_properties(PyDict::New(isolate));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::New(isolate, "dict_values"));

  return JustVoid();
}

void PyDictValuesKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_values_klass(Tagged<PyDictValuesKlass>::null());
}

MaybeHandle<PyObject> PyDictValuesKlass::Virtual_Iter(Isolate* isolate,
                                                      Handle<PyObject> self) {
  return isolate->factory()->NewPyDictValueIterator(
      Handle<PyDictValues>::cast(self)->owner(isolate));
}

MaybeHandle<PyObject> PyDictValuesKlass::Virtual_Len(Isolate* isolate,
                                                     Handle<PyObject> self) {
  return DictViewLen<PyDictValues>(isolate, self);
}

Maybe<bool> PyDictValuesKlass::Virtual_Contains(Isolate* isolate,
                                                Handle<PyObject> self,
                                                Handle<PyObject> subscr) {
  HandleScope scope(isolate);

  auto dict = Handle<PyDictValues>::cast(self)->owner(isolate);
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> value = dict->ValueAtIndex(i, isolate);
    if (value.is_null()) {
      continue;
    }

    bool equal;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, equal,
                               PyObject::EqualBool(isolate, value, subscr));

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

Tagged<PyDictItemsKlass> PyDictItemsKlass::GetInstance(Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictItems);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  set_klass_properties(PyDict::New(isolate));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::New(isolate, "dict_items"));

  return JustVoid();
}

void PyDictItemsKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_items_klass(Tagged<PyDictItemsKlass>::null());
}

MaybeHandle<PyObject> PyDictItemsKlass::Virtual_Iter(Isolate* isolate,
                                                     Handle<PyObject> self) {
  return isolate->factory()->NewPyDictItemIterator(
      Handle<PyDictItems>::cast(self)->owner(isolate));
}

MaybeHandle<PyObject> PyDictItemsKlass::Virtual_Len(Isolate* isolate,
                                                    Handle<PyObject> self) {
  return DictViewLen<PyDictItems>(isolate, self);
}

Maybe<bool> PyDictItemsKlass::Virtual_Contains(Isolate* isolate,
                                               Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  HandleScope scope(isolate);

  if (!IsPyTuple(subscr)) {
    return Maybe<bool>(false);
  }

  auto item = Handle<PyTuple>::cast(subscr);
  if (item->length() != 2) {
    return Maybe<bool>(false);
  }

  auto dict = Handle<PyDictItems>::cast(self)->owner(isolate);
  auto key = item->Get(0, isolate);
  auto value = item->Get(1, isolate);

  Handle<PyObject> found_value;
  bool found = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, found,
                             dict->Get(key, found_value, isolate));

  if (!found) {
    return Maybe<bool>(false);
  }

  bool is_equal = false;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                             PyObject::EqualBool(isolate, found_value, value));
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

Tagged<PyDictKeyIteratorKlass> PyDictKeyIteratorKlass::GetInstance(
    Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictKeyIterator);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  auto klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate,
                      PyDictKeyIteratorBuiltinMethods::Install(
                          isolate, klass_properties, type_object(isolate)));

  set_name(PyString::New(isolate, "dict_keyiterator"));

  return JustVoid();
}

void PyDictKeyIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_keyiterator_klass(
      Tagged<PyDictKeyIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictKeyIteratorKlass::Virtual_Iter(
    Isolate* isolate,
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictKeyIteratorKlass::Virtual_Next(
    Isolate* isolate,
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictKeyIterator>(
      isolate, self, [](Isolate* isolate, Handle<PyDict> dict, int64_t index) {
        return dict->KeyAtIndex(index, isolate);
      });
  if (result.is_null()) {
    Runtime_ThrowError(isolate, ExceptionType::kStopIteration);
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

Tagged<PyDictItemIteratorKlass> PyDictItemIteratorKlass::GetInstance(
    Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictItemIterator);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  auto klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate,
                      PyDictItemIteratorBuiltinMethods::Install(
                          isolate, klass_properties, type_object(isolate)));

  set_name(PyString::New(isolate, "dict_itemiterator"));

  return JustVoid();
}

void PyDictItemIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_itemiterator_klass(
      Tagged<PyDictItemIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictItemIteratorKlass::Virtual_Iter(
    Isolate* isolate,
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictItemIteratorKlass::Virtual_Next(
    Isolate* isolate,
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictItemIterator>(
      isolate, self, [](Isolate* isolate, Handle<PyDict> dict, int64_t index) {
        return dict->ItemAtIndex(index, isolate);
      });
  if (result.is_null()) {
    Runtime_ThrowError(isolate, ExceptionType::kStopIteration);
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

Tagged<PyDictValueIteratorKlass> PyDictValueIteratorKlass::GetInstance(
    Isolate* isolate) {
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
  set_native_layout_kind(NativeLayoutKind::kDictValueIterator);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

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
  auto klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  RETURN_ON_EXCEPTION(isolate,
                      PyDictValueIteratorBuiltinMethods::Install(
                          isolate, klass_properties, type_object(isolate)));

  set_name(PyString::New(isolate, "dict_valueiterator"));

  return JustVoid();
}

void PyDictValueIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_valueiterator_klass(
      Tagged<PyDictValueIteratorKlass>::null());
}

MaybeHandle<PyObject> PyDictValueIteratorKlass::Virtual_Iter(
    Isolate* isolate,
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyDictValueIteratorKlass::Virtual_Next(
    Isolate* isolate,
    Handle<PyObject> self) {
  Handle<PyObject> result = NextFromIterator<PyDictValueIterator>(
      isolate, self, [](Isolate* isolate, Handle<PyDict> dict, int64_t index) {
        return dict->ValueAtIndex(index, isolate);
      });
  if (result.is_null()) {
    Runtime_ThrowError(isolate, ExceptionType::kStopIteration);
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
