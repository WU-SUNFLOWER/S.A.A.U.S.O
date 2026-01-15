// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list.h"

#include <cassert>

#include "src/code/pyc-file-parser.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyList> PyList::NewInstance(int64_t init_capacity) {
  HandleScope scope;

  // 分配对象本体
  Handle<PyList> object(
      Universe::heap_->Allocate<PyList>(Heap::AllocationSpace::kNewSpace));

  // 分配fixed array
  Handle<FixedArray> array =
      FixedArray::NewInstance(std::max(kMinimumCapacity, init_capacity));

  // 设置字段值
  object->length_ = 0;
  object->array_ = *array;

  // 绑定klass
  PyObject::SetKlass(object, PyListKlass::GetInstance());

  return object.EscapeFrom(&scope);
}

// static
Tagged<PyList> PyList::cast(Tagged<PyObject> object) {
  assert(IsPyList(object));
  return Tagged<PyList>::cast(object);
}

/////////////////////////////////////////////////////////////

Handle<PyObject> PyList::Pop() {
  assert(!IsEmpty());

  return Handle<PyObject>(array()->Get(--length_));
}

Handle<PyObject> PyList::Get(int64_t index) const {
  assert(0 <= index && index < length_);

  return Handle<PyObject>(array()->Get(index));
}

Handle<PyObject> PyList::GetLast() const {
  assert(!IsEmpty());

  return Handle<PyObject>(array()->Get(length_ - 1));
}

void PyList::Set(int64_t index, Handle<PyObject> value) {
  assert(0 <= index && index < length_);

  array()->Set(index, *value);
}

void PyList::RemoveByIndex(int64_t index) {
  assert(0 <= index && index < length_);

  for (auto i = index; i < length_ - 1; ++i) {
    array()->Set(i, array()->Get(i + 1));
  }

  --length_;
}

void PyList::Remove(Handle<PyObject> target) {
  auto index = IndexOf(target);
  if (index >= 0) {
    RemoveByIndex(index);
  }
}

void PyList::Clear() {
  length_ = 0;
}

int64_t PyList::capacity() const {
  return array()->capacity();
}

int64_t PyList::IndexOf(Handle<PyObject> target) const {
  for (auto i = 0; i < length(); ++i) {
    if (IsPyTrue(PyObject::Equal(target, Get(i)))) {
      return i;
    }
  }
  return -1;
}

void PyList::Append(Handle<PyList> self, Handle<PyObject> value) {
  HandleScope scope;

  if (self->IsFull()) {
    ExpandImpl(self);
  }

  self->array()->Set(self->length_++,
                     value.IsNull() ? Tagged<PyObject>::Null() : *value);
}

void PyList::Insert(Handle<PyList> self,
                    int64_t index,
                    Handle<PyObject> value) {
  assert(0 <= index && index <= self->length_);

  HandleScope scope;

  if (self->IsFull()) {
    ExpandImpl(self);
  }
  auto old_length = self->length_;
  self->length_ = old_length + 1;

  for (auto i = old_length; i > index; --i) {
    self->array()->Set(i, self->array()->Get(i - 1));
  }

  self->array()->Set(index, value.IsNull() ? Tagged<PyObject>::Null() : *value);
}

void PyList::ExpandImpl(Handle<PyList> list) {
  int64_t new_capacity = std::max(kMinimumCapacity, list->capacity() << 1);

  Handle<FixedArray> old_array(list->array());
  Handle<FixedArray> new_array =
      FixedArray::NewInstance(new_capacity, old_array);

  list->array_ = *new_array;
}

}  // namespace saauso::internal
