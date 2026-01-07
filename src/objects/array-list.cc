// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/array-list.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

namespace {

Object** AllocateArray(int size) {
  return static_cast<Object**>(
      Universe::heap_->AllocateRaw(size * sizeof(Object*)));
}

}  // namespace

// static

ArrayList* ArrayList::NewInstance(int init_capacity) {
  HandleScope scope;

  Handle<ArrayList> object(Universe::heap_->Allocate<ArrayList>());
  object->capacity_ = init_capacity;
  object->length_ = 0;
  object->array_ = AllocateArray(object->capacity_);

  return object.get();
}

void ArrayList::PushBack(Object* value) {
  if (IsFull()) {
    Expand();
  }

  array_[length_++] = value;
}

void ArrayList::Insert(int index, Object* value) {
  assert(0 <= index && index < length_);

  PushBack(nullptr);

  for (int i = length_; index < i; --i) {
    array_[i] = array_[i - 1];
  }

  array_[index] = value;
}

Object* ArrayList::PopBack() {
  assert(!IsEmpty());

  return array_[--length_];
}

Object* ArrayList::Get(int index) const {
  assert(0 <= index && index < length_);

  return array_[index];
}

Object* ArrayList::GetBack() const {
  assert(!IsEmpty());

  return array_[length_ - 1];
}

void ArrayList::Set(int index, Object* value) {
  assert(0 <= index && index < length_);

  array_[index] = value;
}

void ArrayList::Remove(int index) {
  assert(0 <= index && index < length_);

  for (int i = index; i < length_ - 1; ++i) {
    array_[i] = array_[i + 1];
  }

  --length_;
}

void ArrayList::Clear() {
  length_ = 0;
}

void ArrayList::Expand() {
  int new_capacity = capacity_ << 1;
  Object** new_array = AllocateArray(new_capacity);

  for (int i = 0; i < length_; ++i) {
    new_array[i] = array_[i];
  }

  array_ = new_array;
  capacity_ = new_capacity;
}

}  // namespace saauso::internal
