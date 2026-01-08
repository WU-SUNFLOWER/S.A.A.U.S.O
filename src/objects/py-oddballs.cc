// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs.h"

#include <cassert>

#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

///////////////////////////////////////////////////////////////////////////
// Python布尔值

// static
Tagged<PyBoolean> PyBoolean::NewInstance(bool value) {
  // 布尔值生存在永久区，不会被垃圾回收，因此不需要Handle保护
  auto object =
      Universe::heap_->Allocate<PyBoolean>(Heap::AllocationSpace::kMetaSpace);

  object->value_ = value;
  PyObject::SetKlass(object, PyBooleanKlass::GetInstance());

  return object;
}

// static
Tagged<PyBoolean> PyBoolean::Cast(Tagged<PyObject> object) {
  assert(IsPyBoolean(object));
  return Tagged<PyBoolean>::Cast(object);
}

Tagged<PyBoolean> PyBoolean::Reverse() {
  return Universe::ToPyBoolean(!value_);
}

///////////////////////////////////////////////////////////////////////////
// Python空值

// static
Tagged<PyNone> PyNone::NewInstance() {
  // 布尔值生存在永久区，不会被垃圾回收，因此不需要Handle保护
  auto object =
      Universe::heap_->Allocate<PyNone>(Heap::AllocationSpace::kMetaSpace);
  PyObject::SetKlass(object, PyNoneKlass::GetInstance());
  return object;
}

// static
Tagged<PyNone> PyNone::Cast(Tagged<PyObject> object) {
  assert(IsPyNone(object));
  return Tagged<PyNone>::Cast(object);
}

}  // namespace saauso::internal
