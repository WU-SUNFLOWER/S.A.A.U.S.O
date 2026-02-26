// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object-klass.h"


namespace saauso::internal {

// static
Handle<PyTypeObject> PyTypeObject::NewInstance() {
  EscapableHandleScope scope;

  Handle<PyTypeObject> object(
      Isolate::Current()->heap()->Allocate<PyTypeObject>(
          Heap::AllocationSpace::kNewSpace));

  // 绑定klass
  PyObject::SetKlass(object, PyTypeObjectKlass::GetInstance());

  // 初始化properties
  auto properties = PyDict::NewInstance();
  (void)PyDict::PutMaybe(properties, PyString::NewInstance("__dict__"),
                         properties);
  PyObject::SetProperties(*object, *properties);

  // 初始化对象字段
  object->own_klass_ = Tagged<Klass>::null();

  return scope.Escape(object);
}

// static
Tagged<PyTypeObject> PyTypeObject::cast(Tagged<PyObject> object) {
  assert(IsPyTypeObject(object));
  return Tagged<PyTypeObject>::cast(object);
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
