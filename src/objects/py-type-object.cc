// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object.h"

#include "src/heap/heap.h"
#include "src/objects/py-type-object-klass.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyTypeObject> PyTypeObject::NewInstance() {
  auto object =
      Universe::heap_->Allocate<PyTypeObject>(Heap::AllocationSpace::kNewSpace);

  // 绑定klass
  PyObject::SetKlass(object, PyTypeObjectKlass::GetInstance());

  return Handle<PyTypeObject>(object);
}

/////////////////////////////////////////////////////////////////////////////////

void PyTypeObject::BindWithKlass(Tagged<Klass> klass) {
  Handle<PyTypeObject> this_handle(Tagged<PyTypeObject>(this));
  own_klass_ = klass;
  klass->set_type_object(this_handle);
}

Handle<PyList> PyTypeObject::mro() const {
  return own_klass()->mro();
}

}  // namespace saauso::internal
