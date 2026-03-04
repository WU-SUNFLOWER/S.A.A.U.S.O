// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple.h"

#include <algorithm>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple-klass.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Handle<PyTuple> PyTuple::NewInstance(int64_t length) {
  return Isolate::Current()->factory()->NewPyTuple(length);
}

Tagged<PyTuple> PyTuple::cast(Tagged<PyObject> object) {
  assert(IsTupleLike(object));
  return Tagged<PyTuple>(object.ptr());
}

bool PyTuple::IsTupleLike(Tagged<PyObject> object) {
  return IsHeapObject(object) &&
         PyObject::GetKlass(object)->native_layout_kind() ==
             NativeLayoutKind::kTuple;
}

bool PyTuple::IsTupleLike(Handle<PyObject> object) {
  return IsTupleLike(*object);
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

Maybe<int64_t> PyTuple::IndexOf(Handle<PyObject> target) const {
  return IndexOf(target, 0, length());
}

Maybe<int64_t> PyTuple::IndexOf(Handle<PyObject> target,
                                int64_t begin,
                                int64_t end) const {
  for (auto i = begin; i < end; ++i) {
    Maybe<bool> mb = PyObject::EqualBool(target, Get(i));
    if (mb.IsNothing()) {
      return kNullMaybe;
    }
    if (mb.ToChecked()) {
      return Maybe<int64_t>(i);
    }
  }
  return Maybe<int64_t>(kNotFound);
}

}  // namespace saauso::internal
