// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple.h"

#include <algorithm>

#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple-klass.h"
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Handle<PyTuple> PyTuple::NewInstance(int64_t length) {
  assert(0 <= length);

  HandleScope scope;

  size_t object_size = ComputeObjectSize(length);
  Tagged<PyTuple> object(Isolate::Current()->heap()->AllocateRaw(
      object_size, Heap::AllocationSpace::kNewSpace));

  object->length_ = length;
  PyObject::SetProperties(object, Tagged<PyDict>::null());

  for (auto i = 0; i < length; ++i) {
    object->SetInternal(i, Tagged<PyObject>::null());
  }

  PyObject::SetKlass(object, PyTupleKlass::GetInstance());

  return Handle<PyTuple>(object).EscapeFrom(&scope);
}

Handle<PyTuple> PyTuple::NewInstance(Handle<PyList> elements) {
  HandleScope scope;

  auto length = elements->length();
  auto tuple = NewInstance(length);
  for (auto i = 0; i < length; ++i) {
    tuple->SetInternal(i, *elements->Get(i));
  }
  return tuple.EscapeFrom(&scope);
}

Tagged<PyTuple> PyTuple::cast(Tagged<PyObject> object) {
  assert(IsPyTuple(object));
  return Tagged<PyTuple>(object.ptr());
}

size_t PyTuple::ComputeObjectSize(int64_t length) {
  return ObjectSizeAlign(sizeof(PyTuple) + length * sizeof(Tagged<PyObject>));
}

Handle<PyObject> PyTuple::Get(int64_t index) const {
  return handle(GetTagged(index));
}

Tagged<PyObject> PyTuple::GetTagged(int64_t index) const {
  assert(InRangeWithRightOpen(index, static_cast<int64_t>(0), length_));
  return data()[index];
}

void PyTuple::SetInternal(int64_t index, Handle<PyObject> value) {
  SetInternal(index, *value);
}

void PyTuple::SetInternal(int64_t index, Tagged<PyObject> value) {
  assert(InRangeWithRightOpen(index, static_cast<int64_t>(0), length_));
  data()[index] = value;
  WRITE_BARRIER(Tagged<PyObject>(this), &data()[index], value);
}

void PyTuple::ShrinkInternal(int64_t new_length) {
  assert(InRangeWithRightOpen(new_length, static_cast<int64_t>(0), length_));
  length_ = new_length;
}

int64_t PyTuple::IndexOf(Handle<PyObject> target) const {
  return IndexOf(target, 0, length());
}

int64_t PyTuple::IndexOf(Handle<PyObject> target,
                         int64_t begin,
                         int64_t end) const {
  for (auto i = begin; i < end; ++i) {
    if (PyObject::EqualBool(target, Get(i))) {
      return i;
    }
  }
  return kNotFound;
}

}  // namespace saauso::internal
