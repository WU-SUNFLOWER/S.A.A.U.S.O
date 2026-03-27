// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list.h"

#include <cassert>

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"

namespace saauso::internal {

// static
Handle<PyList> PyList::NewInstance(int64_t init_capacity) {
  return Isolate::Current()->factory()->NewPyList(init_capacity);
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

void PyList::SetAndExtendLength(int64_t index, Handle<PyObject> value) {
  assert(0 <= index && index < capacity());

  array()->Set(index, *value);
  length_ = std::max(length_, index + 1);
}

void PyList::RemoveByIndex(int64_t index) {
  assert(0 <= index && index < length_);

  for (auto i = index; i < length_ - 1; ++i) {
    array()->Set(i, array()->Get(i + 1));
  }

  --length_;
}

Maybe<bool> PyList::Remove(Handle<PyObject> target, Isolate* isolate) {
  int64_t index;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, index, IndexOf(target, isolate));

  if (index != kNotFound) {
    RemoveByIndex(index);
    return Maybe<bool>(true);
  }
  return Maybe<bool>(false);
}

void PyList::Clear() {
  length_ = 0;
}

int64_t PyList::capacity() const {
  return array()->capacity();
}

Maybe<int64_t> PyList::IndexOf(Handle<PyObject> target,
                               Isolate* isolate) const {
  return IndexOf(target, 0, length(), isolate);
}

Maybe<int64_t> PyList::IndexOf(Handle<PyObject> target,
                               int64_t begin,
                               int64_t end,
                               Isolate* isolate) const {
  for (auto i = begin; i < end; ++i) {
    bool is_equal = false;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                               PyObject::EqualBool(isolate, target, Get(i)));
    if (is_equal) {
      return Maybe<int64_t>(i);
    }
  }
  return Maybe<int64_t>(kNotFound);
}

void PyList::Append(Handle<PyList> self, Handle<PyObject> value) {
  HandleScope scope;

  if (self->IsFull()) {
    ExpandImpl(self);
  }

  self->array()->Set(self->length_++,
                     value.is_null() ? Tagged<PyObject>::null() : *value);
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

  self->array()->Set(index,
                     value.is_null() ? Tagged<PyObject>::null() : *value);
}

void PyList::ExpandImpl(Handle<PyList> list) {
  int64_t old_capacity = list->capacity();
  int64_t new_capacity = std::max(kMinimumCapacity, old_capacity << 1);

  Handle<FixedArray> old_array(list->array());
  Handle<FixedArray> new_array =
      Isolate::Current()->factory()->CopyFixedArrayAndGrow(
          old_array, new_capacity - old_capacity);

  list->array_ = *new_array;
}

}  // namespace saauso::internal
