// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/factory.h"

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/handles/handle-scopes.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"

namespace saauso::internal {

Address Factory::AllocateRaw(size_t size_in_bytes,
                             Heap::AllocationSpace space) {
  return isolate_->heap()->AllocateRaw(size_in_bytes, space);
}

Handle<PyDict> Factory::NewPyDict(int64_t init_capacity) {
  EscapableHandleScope scope;
  return scope.Escape(
      AllocateDictLike(PyDictKlass::GetInstance(), init_capacity, false));
}

Handle<PyDict> Factory::AllocateDictLike(Tagged<Klass> klass_self,
                                         int64_t init_capacity,
                                         bool allocate_properties_dict) {
  assert((init_capacity & (init_capacity - 1)) == 0);

  EscapableHandleScope scope;
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->occupied_ = 0;
    PyDict::SetKlass(object, klass_self);
    object->data_ = Tagged<FixedArray>::null();
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<FixedArray> data = NewFixedArray(init_capacity * 2);
  object->data_ = *data;

  if (allocate_properties_dict) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  return scope.Escape(object);
}

Handle<PyDict> Factory::NewPyDictWithoutAllocateData() {
  auto klass = PyDictKlass::GetInstance();
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->occupied_ = 0;
    PyDict::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  return object;
}

Handle<FixedArray> Factory::NewFixedArray(int64_t capacity) {
  assert(0 <= capacity);

  EscapableHandleScope scope;

  size_t object_size = FixedArray::ComputeObjectSize(capacity);
  auto klass = FixedArrayKlass::GetInstance();
  Tagged<FixedArray> object(
      AllocateRaw(object_size, Heap::AllocationSpace::kNewSpace));

  object->capacity_ = capacity;
  {
    DisallowHeapAllocation disallow(isolate_);
    object->capacity_ = capacity;
    PyObject::SetProperties(object, Tagged<PyDict>::null());
    for (int64_t i = 0; i < capacity; ++i) {
      object->Set(i, Tagged<PyObject>(kNullAddress));
    }
    PyObject::SetKlass(object, klass);
  }
  return scope.Escape(Handle<FixedArray>(object));
}

Handle<PyFloat> Factory::NewPyFloat(double value) {
  EscapableHandleScope scope;

  auto klass = PyFloatKlass::GetInstance();
  Handle<PyFloat> object(Allocate<PyFloat>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_value(value);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  return scope.Escape(object);
}

Handle<PyFunction> Factory::NewPyFunction() {
  EscapableHandleScope scope;

  auto klass = PyFunctionKlass::GetInstance();
  Handle<PyFunction> object(
      Allocate<PyFunction>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->func_code_ = Tagged<PyObject>::null();
    object->func_name_ = Tagged<PyObject>::null();
    object->func_globals_ = Tagged<PyObject>::null();
    object->default_args_ = Tagged<PyObject>::null();
    object->closures_ = Tagged<PyObject>::null();
    object->flags_ = 0;
    object->native_func_ = nullptr;
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  (void)PyDict::Put(properties, PyString::NewInstance("__dict__"), properties);
  PyObject::SetProperties(*object, *properties);

  return scope.Escape(object);
}

Handle<MethodObject> Factory::NewMethodObject(Handle<PyObject> func,
                                              Handle<PyObject> owner) {
  auto klass = MethodObjectKlass::GetInstance();
  Handle<MethodObject> object(
      Allocate<MethodObject>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->owner_ = Tagged<PyObject>::null();
    object->func_ = Tagged<PyObject>::null();
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  object->owner_ = *owner;
  object->func_ = *func;
  return object;
}

MaybeHandle<PyModule> Factory::NewPyModule() {
  EscapableHandleScope scope;

  auto klass = PyModuleKlass::GetInstance();
  Handle<PyModule> object(Allocate<PyModule>(Heap::AllocationSpace::kNewSpace));

  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  RETURN_ON_EXCEPTION(
      isolate_,
      PyDict::Put(properties, PyString::NewInstance("__dict__"), properties));

  PyObject::SetProperties(*object, *properties);

  return scope.Escape(object);
}

Handle<PyTypeObject> Factory::NewPyTypeObject() {
  EscapableHandleScope scope;

  auto klass = PyTypeObjectKlass::GetInstance();
  Handle<PyTypeObject> object(
      Allocate<PyTypeObject>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    object->own_klass_ = Tagged<Klass>::null();
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  (void)PyDict::Put(properties, PyString::NewInstance("__dict__"), properties);
  PyObject::SetProperties(*object, *properties);

  return scope.Escape(object);
}

Handle<PyObject> Factory::AllocateRawPythonObject() {
  EscapableHandleScope scope;

  auto klass = PyObjectKlass::GetInstance();
  Handle<PyObject> object(Allocate<PyObject>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  PyObject::SetProperties(*object, *properties);
  return scope.Escape(object);
}

Tagged<PyBoolean> Factory::NewPyBoolean(bool value) {
  auto klass = PyBooleanKlass::GetInstance();
  auto object = Allocate<PyBoolean>(Heap::AllocationSpace::kMetaSpace);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->value_ = value;
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(object, Tagged<PyDict>::null());
  }
  return object;
}

Tagged<PyNone> Factory::NewPyNone() {
  auto klass = PyNoneKlass::GetInstance();
  auto object = Allocate<PyNone>(Heap::AllocationSpace::kMetaSpace);
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(object, Tagged<PyDict>::null());
  }
  return object;
}

}  // namespace saauso::internal
