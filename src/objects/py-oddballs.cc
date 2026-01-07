// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-oddballs.h"

#include <cassert>

#include "heap/heap.h"
#include "objects/py-oddballs-klass.h"
#include "runtime/universe.h"

namespace saauso::internal {

///////////////////////////////////////////////////////////////////////////
// Python布尔值

// static
PyBoolean* PyBoolean::NewInstance(bool value) {
  // 布尔值生存在永久区，不会被垃圾回收，因此不需要Handle保护
  PyBoolean* object =
      Universe::heap_->Allocate<PyBoolean>(Heap::AllocationSpace::kMetaSpace);

  object->value_ = value;
  object->set_klass(PyBooleanKlass::GetInstance());

  return object;
}

// static
PyBoolean* PyBoolean::Cast(PyObject* object) {
  assert(object->IsPyBoolean());
  return reinterpret_cast<PyBoolean*>(object);
}

PyBoolean* PyBoolean::Reverse() {
  return Universe::ToPyBoolean(!value_);
}

///////////////////////////////////////////////////////////////////////////
// Python空值

// static
PyNone* PyNone::NewInstance(bool value) {
  // 布尔值生存在永久区，不会被垃圾回收，因此不需要Handle保护
  PyNone* object =
      Universe::heap_->Allocate<PyNone>(Heap::AllocationSpace::kMetaSpace);
  object->set_klass(PyNoneKlass::GetInstance());
  return object;
}

// static
PyNone* PyNone::Cast(PyObject* object) {
  assert(object->IsPyNone());
  return reinterpret_cast<PyNone*>(object);
}

}  // namespace saauso::internal
