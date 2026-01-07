// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-float-klass.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyFloat> PyFloat::NewInstance(double value) {
  Handle<PyFloat> object(
      Universe::heap_->Allocate<PyFloat>(Heap::AllocationSpace::kNewSpace));
  object->value_ = value;

  // 绑定klass
  object->set_klass(PyFloatKlass::GetInstance());

  return object;
}

// static
PyFloat* PyFloat::Cast(PyObject* object) {
  assert(object->IsPyFloat());
  return reinterpret_cast<PyFloat*>(object);
}

}  // namespace saauso::internal
