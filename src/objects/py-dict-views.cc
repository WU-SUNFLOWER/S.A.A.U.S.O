// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-views.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

Handle<PyDictKeys> PyDictKeys::NewInstance(Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictKeys> view(Isolate::Current()->heap()->Allocate<PyDictKeys>(
      Heap::AllocationSpace::kNewSpace));
  view->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;

  PyObject::SetKlass(view, PyDictKeysKlass::GetInstance());
  PyObject::SetProperties(*view, Tagged<PyDict>::Null());

  return view.EscapeFrom(&scope);
}

Tagged<PyDictKeys> PyDictKeys::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeys(object));
  return Tagged<PyDictKeys>::cast(object);
}

Handle<PyDict> PyDictKeys::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

Handle<PyDictValues> PyDictValues::NewInstance(Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictValues> view(Isolate::Current()->heap()->Allocate<PyDictValues>(
      Heap::AllocationSpace::kNewSpace));
  view->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;

  PyObject::SetKlass(view, PyDictValuesKlass::GetInstance());
  PyObject::SetProperties(*view, Tagged<PyDict>::Null());

  return view.EscapeFrom(&scope);
}

Tagged<PyDictValues> PyDictValues::cast(Tagged<PyObject> object) {
  assert(IsPyDictValues(object));
  return Tagged<PyDictValues>::cast(object);
}

Handle<PyDict> PyDictValues::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

Handle<PyDictItems> PyDictItems::NewInstance(Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictItems> view(Isolate::Current()->heap()->Allocate<PyDictItems>(
      Heap::AllocationSpace::kNewSpace));
  view->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;

  PyObject::SetKlass(view, PyDictItemsKlass::GetInstance());
  PyObject::SetProperties(*view, Tagged<PyDict>::Null());

  return view.EscapeFrom(&scope);
}

Tagged<PyDictItems> PyDictItems::cast(Tagged<PyObject> object) {
  assert(IsPyDictItems(object));
  return Tagged<PyDictItems>::cast(object);
}

Handle<PyDict> PyDictItems::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

Handle<PyDictKeyIterator> PyDictKeyIterator::NewInstance(
    Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictKeyIterator> iterator(
      Isolate::Current()->heap()->Allocate<PyDictKeyIterator>(
          Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;
  iterator->iter_index_ = 0;

  PyObject::SetKlass(iterator, PyDictKeyIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::Null());

  return iterator.EscapeFrom(&scope);
}

Tagged<PyDictKeyIterator> PyDictKeyIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictKeyIterator(object));
  return Tagged<PyDictKeyIterator>::cast(object);
}

Handle<PyDict> PyDictKeyIterator::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

Handle<PyDictValueIterator> PyDictValueIterator::NewInstance(
    Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictValueIterator> iterator(
      Isolate::Current()->heap()->Allocate<PyDictValueIterator>(
          Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;
  iterator->iter_index_ = 0;

  PyObject::SetKlass(iterator, PyDictValueIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::Null());

  return iterator.EscapeFrom(&scope);
}

Tagged<PyDictValueIterator> PyDictValueIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictValueIterator(object));
  return Tagged<PyDictValueIterator>::cast(object);
}

Handle<PyDict> PyDictValueIterator::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

Handle<PyDictItemIterator> PyDictItemIterator::NewInstance(
    Handle<PyObject> owner) {
  HandleScope scope;

  Handle<PyDictItemIterator> iterator(
      Isolate::Current()->heap()->Allocate<PyDictItemIterator>(
          Heap::AllocationSpace::kNewSpace));

  iterator->owner_ = owner.IsNull() ? Tagged<PyObject>::Null() : *owner;
  iterator->iter_index_ = 0;

  PyObject::SetKlass(iterator, PyDictItemIteratorKlass::GetInstance());
  PyObject::SetProperties(*iterator, Tagged<PyDict>::Null());

  return iterator.EscapeFrom(&scope);
}

Tagged<PyDictItemIterator> PyDictItemIterator::cast(Tagged<PyObject> object) {
  assert(IsPyDictItemIterator(object));
  return Tagged<PyDictItemIterator>::cast(object);
}

Handle<PyDict> PyDictItemIterator::owner() const {
  assert(!owner_.IsNull());
  return handle(Tagged<PyDict>::cast(owner_));
}

}  // namespace saauso::internal
