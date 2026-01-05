// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-list.h"

#include <cassert>

#include "code/pyc-file-parser.h"
#include "handles/handles.h"
#include "heap/heap.h"
#include "objects/py-object.h"
#include "runtime/universe.h"

namespace saauso::internal {

namespace {

PyObject** AllocateArray(int size) {
  return static_cast<PyObject**>(
      Universe::heap_->AllocateRaw(size * sizeof(PyObject*)));
}

}  // namespace

// static
Handle<PyList> PyList::NewInstance(int init_capacity) {
  Handle<PyList> object(Universe::heap_->Allocate<PyList>());
  object->capacity_ = init_capacity;
  object->length_ = 0;
  object->array_ = AllocateArray(object->capacity_);

  return object;
}

// static
PyList* PyList::Cast(PyObject* object) {
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

void PyList::Set(int index, PyObject* value) {
  assert(0 <= index && index < length_);

  array_[index] = value;
  WRITE_BARRIER;
}

void PyList::Remove(int index) {
  assert(0 <= index && index < length_);

  for (int i = index; i < length_ - 1; ++i) {
    array_[i] = array_[i + 1];
  }

  --length_;
}

void PyList::Clear() {
  length_ = 0;
}

// static
void PyList::Append(Handle<PyList> list, Handle<PyObject> value) {
  if (list->IsFull()) {
    ExpandImpl(list);
  }

  list->array_[list->length_++] = value.IsNull() ? nullptr : *value;
  WRITE_BARRIER;
}

// static
void PyList::Insert(Handle<PyList> list, int index, Handle<PyObject> value) {
  if (list->IsFull()) {
    ExpandImpl(list);
  }
  ++list->length_;

  for (int i = list->length_; i > index; --i) {
    list->array_[i] = list->array_[i - 1];
  }

  list->array_[index] = value.IsNull() ? nullptr : *value;
  WRITE_BARRIER;
}

// static
void PyList::ExpandImpl(Handle<PyList> list) {
  int new_capacity = list->capacity_ << 1;
  PyObject** new_array = AllocateArray(new_capacity);

  for (int i = 0; i < list->length_; ++i) {
    new_array[i] = list->array_[i];
  }

  list->array_ = new_array;
  WRITE_BARRIER;
  list->capacity_ = new_capacity;
}

}  // namespace saauso::internal
