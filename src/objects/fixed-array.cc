// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/fixed-array.h"

#include <cstring>

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/py-object.h"
#include "src/utils/utils.h"

namespace saauso::internal {

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
