// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs.h"

#include <cassert>

#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"


namespace saauso::internal {

///////////////////////////////////////////////////////////////////////////
// Python布尔值

// static
Tagged<PyBoolean> PyBoolean::NewInstance(bool value) {
  // 布尔值生存在永久区，不会被垃圾回收，因此不需要Handle保护
  auto object = Isolate::Current()->heap()->Allocate<PyBoolean>(
      Heap::AllocationSpace::kMetaSpace);

  object->value_ = value;
  PyObject::SetKlass(object, PyBooleanKlass::GetInstance());

  // bool类型没有__dict__，不需要初始化properties
  // >>> True.__dict__
  // Traceback (most recent call last):
  //   File "<stdin>", line 1, in <module>
  // AttributeError: 'bool' object has no attribute '__dict__'
  PyObject::SetProperties(object, Tagged<PyDict>::null());

  return object;
}

// static
Tagged<PyBoolean> PyBoolean::cast(Tagged<PyObject> object) {
  assert(IsPyBoolean(object));
  return Tagged<PyBoolean>::cast(object);
}

Tagged<PyBoolean> PyBoolean::Reverse() {
  return Isolate::ToPyBoolean(!value_);
}

///////////////////////////////////////////////////////////////////////////
// Python空值

// static
Tagged<PyNone> PyNone::NewInstance() {
  // none生存在永久区，不会被垃圾回收，因此不需要Handle保护
  auto object = Isolate::Current()->heap()->Allocate<PyNone>(
      Heap::AllocationSpace::kMetaSpace);
  PyObject::SetKlass(object, PyNoneKlass::GetInstance());

  // NoneType类型没有__dict__，不需要初始化properties
  // >>> None.__dict__
  // Traceback (most recent call last):
  //   File "<stdin>", line 1, in <module>
  // AttributeError: 'NoneType' object has no attribute '__dict__'
  PyObject::SetProperties(object, Tagged<PyDict>::null());

  return object;
}

// static
Tagged<PyNone> PyNone::cast(Tagged<PyObject> object) {
  assert(IsPyNone(object));
  return Tagged<PyNone>::cast(object);
}

}  // namespace saauso::internal
