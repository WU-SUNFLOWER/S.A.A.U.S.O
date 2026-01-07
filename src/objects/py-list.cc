// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list.h"

#include <cassert>

#include "src/code/pyc-file-parser.h"
#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-object.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

namespace {

PyObject** AllocateArray(int size) {
  return static_cast<PyObject**>(Universe::heap_->AllocateRaw(
      size * sizeof(PyObject*), Heap::AllocationSpace::kNewSpace));
}

}  // namespace

// static
Handle<PyList> PyList::NewInstance(int64_t init_capacity) {
  Handle<PyList> object(
      Universe::heap_->Allocate<PyList>(Heap::AllocationSpace::kNewSpace));
  object->capacity_ = init_capacity;
  object->length_ = 0;

  // AllocateArray会触发GC，
  // 在此之前先手工将array_置为空，避免GC尝试访问一个悬空指针
  object->array_ = nullptr;
  if (object->capacity_ > 0) [[likely]] {
    object->array_ = AllocateArray(object->capacity_);
  }

  // 绑定klass
  object->set_klass(PyListKlass::GetInstance());

  return object;
}

// static
PyList* PyList::Cast(PyObject* object) {
  assert(object->IsPyList());
  return reinterpret_cast<PyList*>(object);
}

/////////////////////////////////////////////////////////////

Handle<PyObject> PyList::Pop() {
  assert(!IsEmpty());

  return Handle<PyObject>(array_[--length_]);
}

Handle<PyObject> PyList::Get(int index) const {
  assert(0 <= index && index < length_);

  return Handle<PyObject>(array_[index]);
}

Handle<PyObject> PyList::GetLast() const {
  assert(!IsEmpty());

  return Handle<PyObject>(array_[length_ - 1]);
}

void PyList::Set(int64_t index, Handle<PyObject> value) {
  assert(0 <= index && index < length_);

  array_[index] = *value;
  WRITE_BARRIER;
}

void PyList::Remove(int64_t index) {
  assert(0 <= index && index < length_);

  for (auto i = index; i < length_ - 1; ++i) {
    array_[i] = array_[i + 1];
  }

  --length_;
}

void PyList::Clear() {
  length_ = 0;
}

void PyList::Append(Handle<PyObject> value) {
  HandleScope scope;
  Handle<PyList> this_handle(this);

  if (this_handle->IsFull()) {
    ExpandImpl(this_handle);
  }

  this_handle->array_[this_handle->length_++] =
      value.IsNull() ? nullptr : *value;
  WRITE_BARRIER;
}

void PyList::Insert(int64_t index, Handle<PyObject> value) {
  assert(0 <= index && index < length_);

  HandleScope scope;
  Handle<PyList> this_handle(this);

  if (this_handle->IsFull()) {
    ExpandImpl(this_handle);
  }
  ++this_handle->length_;

  for (auto i = this_handle->length_; i > index; --i) {
    this_handle->array_[i] = this_handle->array_[i - 1];
  }

  this_handle->array_[index] = value.IsNull() ? nullptr : *value;
  WRITE_BARRIER;
}

void PyList::ExpandImpl(Handle<PyList> list) {
  int64_t new_capacity =
      std::max(kDefaultInitialCapacity, list->capacity_ << 1);
  PyObject** new_array = AllocateArray(new_capacity);

  for (auto i = 0; i < list->length_; ++i) {
    new_array[i] = list->array_[i];
  }

  list->array_ = new_array;
  WRITE_BARRIER;
  list->capacity_ = new_capacity;
}

}  // namespace saauso::internal
