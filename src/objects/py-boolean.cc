// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "objects/py-boolean.h"

#include <cassert>

#include "heap/heap.h"
#include "objects/py-boolean-klass.h"
#include "runtime/universe.h"

namespace saauso::internal {

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

}  // namespace saauso::internal
