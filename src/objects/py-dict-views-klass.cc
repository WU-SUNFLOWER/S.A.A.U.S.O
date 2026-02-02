// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-views-klass.h"

#include <cassert>
#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

template <typename IteratorType, typename Getter>
Handle<PyObject> NextFromIterator(Handle<PyObject> self, Getter getter) {
  HandleScope scope;

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
    return result.EscapeFrom(&scope);
  }

  iterator->set_iter_index(dict->capacity());
  return Handle<PyObject>::null();
}

template <typename ViewType>
Handle<PyObject> DictViewLen(Handle<PyObject> self) {
  auto dict = Handle<ViewType>::cast(self)->owner();
  return Handle<PyObject>(PySmi::FromInt(dict->occupied()));
}

Handle<PyObject> Native_KeyIteratorNext(Handle<PyObject> self,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs) {
  HandleScope scope;
  return NextFromIterator<PyDictKeyIterator>(
             self, [](Handle<PyDict> dict,
                      int64_t index) { return dict->KeyAtIndex(index); })
      .EscapeFrom(&scope);
}

Handle<PyObject> Native_ValueIteratorNext(Handle<PyObject> self,
                                          Handle<PyTuple> args,
                                          Handle<PyDict> kwargs) {
  HandleScope scope;
  return NextFromIterator<PyDictValueIterator>(
             self, [](Handle<PyDict> dict,
                      int64_t index) { return dict->ValueAtIndex(index); })
      .EscapeFrom(&scope);
}

Handle<PyObject> Native_ItemIteratorNext(Handle<PyObject> self,
                                         Handle<PyTuple> args,
                                         Handle<PyDict> kwargs) {
  HandleScope scope;
  return NextFromIterator<PyDictItemIterator>(
             self, [](Handle<PyDict> dict,
                      int64_t index) { return dict->ItemAtIndex(index); })
      .EscapeFrom(&scope);
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

void PyDictKeysKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.len = &Virtual_Len;
  vtable_.contains = &Virtual_Contains;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictKeysKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_keys"));
}

void PyDictKeysKlass::Finalize() {
  Isolate::Current()->set_py_dict_keys_klass(Tagged<PyDictKeysKlass>::null());
}

void PyDictKeysKlass::Virtual_Print(Handle<PyObject> self) {
  auto view = Handle<PyDictKeys>::cast(self);
  auto dict = view->owner();

  std::printf("dict_keys([");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> key = dict->KeyAtIndex(i);
    if (key.is_null()) {
      continue;
    }
    if (!first) {
      std::printf(", ");
    }
    first = false;
    PyObject::Print(key);
  }
  std::printf("])");
}

Handle<PyObject> PyDictKeysKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyDictKeyIterator::NewInstance(
      Handle<PyDictKeys>::cast(self)->owner());
}

Handle<PyObject> PyDictKeysKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictKeys>(self);
}

bool PyDictKeysKlass::Virtual_Contains(Handle<PyObject> self,
                                       Handle<PyObject> subscr) {
  auto dict = Handle<PyDictKeys>::cast(self)->owner();
  return dict->Contains(subscr);
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

void PyDictValuesKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.len = &Virtual_Len;
  vtable_.contains = &Virtual_Contains;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictValuesKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_values"));
}

void PyDictValuesKlass::Finalize() {
  Isolate::Current()->set_py_dict_values_klass(
      Tagged<PyDictValuesKlass>::null());
}

void PyDictValuesKlass::Virtual_Print(Handle<PyObject> self) {
  auto view = Handle<PyDictValues>::cast(self);
  auto dict = view->owner();

  std::printf("dict_values([");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> value = dict->ValueAtIndex(i);
    if (value.is_null()) {
      continue;
    }
    if (!first) {
      std::printf(", ");
    }
    first = false;
    PyObject::Print(value);
  }
  std::printf("])");
}

Handle<PyObject> PyDictValuesKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyDictValueIterator::NewInstance(
      Handle<PyDictValues>::cast(self)->owner());
}

Handle<PyObject> PyDictValuesKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictValues>(self);
}

bool PyDictValuesKlass::Virtual_Contains(Handle<PyObject> self,
                                         Handle<PyObject> subscr) {
  HandleScope scope;

  auto dict = Handle<PyDictValues>::cast(self)->owner();
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> value = dict->ValueAtIndex(i);
    if (value.is_null()) {
      continue;
    }
    if (PyObject::EqualBool(value, subscr)) {
      return true;
    }
  }
  return false;
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

void PyDictItemsKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.len = &Virtual_Len;
  vtable_.contains = &Virtual_Contains;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictItemsKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_items"));
}

void PyDictItemsKlass::Finalize() {
  Isolate::Current()->set_py_dict_items_klass(Tagged<PyDictItemsKlass>::null());
}

void PyDictItemsKlass::Virtual_Print(Handle<PyObject> self) {
  auto view = Handle<PyDictItems>::cast(self);
  auto dict = view->owner();

  std::printf("dict_items([");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    Handle<PyObject> item = dict->ItemAtIndex(i);
    if (item.is_null()) {
      continue;
    }
    if (!first) {
      std::printf(", ");
    }
    first = false;
    PyObject::Print(item);
  }
  std::printf("])");
}

Handle<PyObject> PyDictItemsKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyDictItemIterator::NewInstance(
      Handle<PyDictItems>::cast(self)->owner());
}

Handle<PyObject> PyDictItemsKlass::Virtual_Len(Handle<PyObject> self) {
  return DictViewLen<PyDictItems>(self);
}

bool PyDictItemsKlass::Virtual_Contains(Handle<PyObject> self,
                                        Handle<PyObject> subscr) {
  HandleScope scope;

  if (!IsPyTuple(subscr)) {
    return false;
  }

  auto item = Handle<PyTuple>::cast(subscr);
  if (item->length() != 2) {
    return false;
  }

  auto dict = Handle<PyDictItems>::cast(self)->owner();
  auto key = item->Get(0);
  auto value = item->Get(1);

  auto found = dict->Get(key);
  if (found.is_null()) {
    return false;
  }

  return PyObject::EqualBool(found, value);
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

Handle<PyObject> PyDictKeyIteratorKlass::Native_Next(Handle<PyObject> self,
                                                     Handle<PyTuple> args,
                                                     Handle<PyDict> kwargs) {
  return Native_KeyIteratorNext(self, args, kwargs);
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

void PyDictKeyIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictKeyIteratorKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  auto klass_properties = PyDict::NewInstance();
  PyDict::Put(klass_properties, ST(next),
              PyFunction::NewInstance(&Native_Next, ST(next)));
  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_keyiterator"));
}

void PyDictKeyIteratorKlass::Finalize() {
  Isolate::Current()->set_py_dict_keyiterator_klass(
      Tagged<PyDictKeyIteratorKlass>::null());
}

void PyDictKeyIteratorKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("<dict_keyiterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> PyDictKeyIteratorKlass::Virtual_Iter(Handle<PyObject> self) {
  return self;
}

Handle<PyObject> PyDictKeyIteratorKlass::Virtual_Next(Handle<PyObject> self) {
  return NextFromIterator<PyDictKeyIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->KeyAtIndex(index);
      });
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

Handle<PyObject> PyDictItemIteratorKlass::Native_Next(Handle<PyObject> self,
                                                      Handle<PyTuple> args,
                                                      Handle<PyDict> kwargs) {
  return Native_ItemIteratorNext(self, args, kwargs);
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

void PyDictItemIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictItemIteratorKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  auto klass_properties = PyDict::NewInstance();
  PyDict::Put(klass_properties, ST(next),
              PyFunction::NewInstance(&Native_Next, ST(next)));
  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_itemiterator"));
}

void PyDictItemIteratorKlass::Finalize() {
  Isolate::Current()->set_py_dict_itemiterator_klass(
      Tagged<PyDictItemIteratorKlass>::null());
}

void PyDictItemIteratorKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("<dict_itemiterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> PyDictItemIteratorKlass::Virtual_Iter(Handle<PyObject> self) {
  return self;
}

Handle<PyObject> PyDictItemIteratorKlass::Virtual_Next(Handle<PyObject> self) {
  return NextFromIterator<PyDictItemIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->ItemAtIndex(index);
      });
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

Handle<PyObject> PyDictValueIteratorKlass::Native_Next(Handle<PyObject> self,
                                                       Handle<PyTuple> args,
                                                       Handle<PyDict> kwargs) {
  return Native_ValueIteratorNext(self, args, kwargs);
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

void PyDictValueIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictValueIteratorKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  auto klass_properties = PyDict::NewInstance();
  PyDict::Put(klass_properties, ST(next),
              PyFunction::NewInstance(&Native_Next, ST(next)));
  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_valueiterator"));
}

void PyDictValueIteratorKlass::Finalize() {
  Isolate::Current()->set_py_dict_valueiterator_klass(
      Tagged<PyDictValueIteratorKlass>::null());
}

void PyDictValueIteratorKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("<dict_valueiterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> PyDictValueIteratorKlass::Virtual_Iter(Handle<PyObject> self) {
  return self;
}

Handle<PyObject> PyDictValueIteratorKlass::Virtual_Next(Handle<PyObject> self) {
  return NextFromIterator<PyDictValueIterator>(
      self, [](Handle<PyDict> dict, int64_t index) {
        return dict->ValueAtIndex(index);
      });
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
