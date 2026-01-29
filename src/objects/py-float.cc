// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-float.h"

#include <cassert>

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-float-klass.h"
#include "src/runtime/isolate.h"

namespace saauso::internal {

// static
Handle<PyFloat> PyFloat::NewInstance(double value) {
  Handle<PyFloat> object(Isolate::Current()->heap()->Allocate<PyFloat>(
      Heap::AllocationSpace::kNewSpace));
  object->value_ = value;

  // 绑定klass
  PyObject::SetKlass(object, PyFloatKlass::GetInstance());

  // float类型没有__dict__，不需要初始化properties
  // >>> 1.234.__dict__
  // Traceback (most recent call last):
  //   File "<stdin>", line 1, in <module>
  // AttributeError: 'float' object has no attribute '__dict__'
  PyObject::SetProperties(*object, Tagged<PyDict>::null());

  return object;
}

// static
Tagged<PyFloat> PyFloat::cast(Tagged<PyObject> object) {
  assert(IsPyFloat(object));
  return Tagged<PyFloat>::cast(object);
}

}  // namespace saauso::internal
