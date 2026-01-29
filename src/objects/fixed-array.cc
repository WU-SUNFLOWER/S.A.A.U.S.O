// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/fixed-array.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/py-object.h"
#include "src/runtime/isolate.h"
#include "src/utils/utils.h"

namespace saauso::internal {

// static
Handle<FixedArray> FixedArray::NewInstance(int64_t capacity) {
  size_t object_size = ComputeObjectSize(capacity);
  Tagged<FixedArray> object(Isolate::Current()->heap()->AllocateRaw(
      object_size, Heap::AllocationSpace::kNewSpace));

  // 设置字段
  object->capacity_ = capacity;
  PyObject::SetProperties(object, Tagged<PyDict>::null());

  // 清空数据区
  for (auto i = 0; i < capacity; ++i) {
    object->Set(i, Tagged<PyObject>(kNullAddress));
  }

  // 绑定klass
  PyObject::SetKlass(object, FixedArrayKlass::GetInstance());

  return Handle<FixedArray>(object);
}

// static
Handle<FixedArray> FixedArray::NewInstance(int64_t capacity,
                                           Handle<FixedArray> copied_source) {
  // 新创建的fixed array的容量不能比拷贝对象还小
  assert(copied_source->capacity() <= capacity);

  Handle<FixedArray> result = NewInstance(capacity);
  std::memcpy(result->data(), copied_source->data(),
              copied_source->capacity() * sizeof(Tagged<PyObject>));

  return result;
}

// static
Handle<FixedArray> FixedArray::Clone(Handle<FixedArray> other) {
  return NewInstance(other->capacity(), other);
}

// static
Tagged<FixedArray> FixedArray::cast(Tagged<PyObject> object) {
  assert(IsFixedArray(object));
  return Tagged<FixedArray>(object.ptr());
}

// static
size_t FixedArray::ComputeObjectSize(int64_t capacity) {
  return ObjectSizeAlign(sizeof(FixedArray) +
                         capacity * sizeof(Tagged<PyObject>));
}

////////////////////////////////////////////////////////////////////////////

Tagged<PyObject> FixedArray::Get(int64_t index) {
  assert(InRangeWithRightOpen(index, static_cast<int64_t>(0), capacity_));
  return data()[index];
}

void FixedArray::Set(int64_t index, Handle<PyObject> value) {
  Set(index, *value);
}

void FixedArray::Set(int64_t index, Tagged<PyObject> value) {
  assert(InRangeWithRightOpen(index, static_cast<int64_t>(0), capacity_));
  data()[index] = value;
  WRITE_BARRIER(Tagged<PyObject>(this), &data()[index], value);
}

}  // namespace saauso::internal
