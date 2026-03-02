// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/factory.h"

#include <algorithm>

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/handles/handle-scopes.h"
#include "src/objects/cell-klass.h"
#include "src/objects/cell.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/fixed-array.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Address Factory::AllocateRaw(size_t size_in_bytes,
                             Heap::AllocationSpace space) {
  return isolate_->heap()->AllocateRaw(size_in_bytes, space);
}

Handle<PyDict> Factory::NewPyDict(int64_t init_capacity) {
  EscapableHandleScope scope;
  return scope.Escape(
      NewDictLike(PyDictKlass::GetInstance(), init_capacity, false));
}

Handle<PyDict> Factory::NewDictLike(Tagged<Klass> klass_self,
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

Handle<Cell> Factory::NewCell() {
  Handle<Cell> object(Allocate<Cell>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
    object->value_ = Tagged<PyObject>(kNullAddress);
    PyObject::SetKlass(object, CellKlass::GetInstance());
  }
  return object;
}

Tagged<Klass> Factory::CreateRawPythonKlass() {
  auto klass = Allocate<Klass>(Heap::AllocationSpace::kMetaSpace);
  {
    DisallowHeapAllocation disallow(isolate_);
    klass->InitializeVTable();
    klass->set_klass_properties(Handle<PyDict>::null());
    klass->set_name(Handle<PyString>::null());
    klass->set_type_object(Handle<PyTypeObject>::null());
    klass->set_supers(Handle<PyList>::null());
    klass->set_mro(Handle<PyList>::null());
    klass->set_native_layout_base(Tagged<Klass>::null());
    klass->set_native_layout_kind(NativeLayoutKind::kPyObject);
  }
  return klass;
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

Handle<PyList> Factory::NewPyListLike(Tagged<Klass> klass_self,
                                      int64_t init_capacity,
                                      bool allocate_properties_dict) {
  EscapableHandleScope scope;

  Handle<PyList> object(Allocate<PyList>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->length_ = 0;
    object->array_ = Tagged<FixedArray>::null();
    PyObject::SetKlass(object, klass_self);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<FixedArray> array =
      NewFixedArray(std::max(PyList::kMinimumCapacity, init_capacity));
  object->array_ = *array;

  if (allocate_properties_dict) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  return scope.Escape(object);
}

Handle<PyList> Factory::NewPyList(int64_t init_capacity) {
  EscapableHandleScope scope;
  return scope.Escape(
      NewPyListLike(PyListKlass::GetInstance(), init_capacity, false));
}

Handle<PyTuple> Factory::NewPyTupleLike(Tagged<Klass> klass_self,
                                        int64_t length,
                                        bool allocate_properties_dict) {
  assert(0 <= length);

  EscapableHandleScope scope;

  size_t object_size = PyTuple::ComputeObjectSize(length);
  Tagged<PyTuple> object(
      AllocateRaw(object_size, Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->length_ = length;
    PyObject::SetKlass(object, klass_self);
    PyObject::SetProperties(object, Tagged<PyDict>::null());
    for (int64_t i = 0; i < length; ++i) {
      object->SetInternal(i, Tagged<PyObject>::null());
    }
  }

  if (allocate_properties_dict) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(object, *properties);
  }

  return scope.Escape(handle(object));
}

Handle<PyTuple> Factory::NewPyTuple(int64_t length) {
  return NewPyTupleLike(PyTupleKlass::GetInstance(), length, false);
}

Handle<PyTuple> Factory::NewPyTupleWithElements(Handle<PyList> elements) {
  EscapableHandleScope scope;
  auto length = elements->length();
  auto tuple = NewPyTuple(length);
  for (auto i = 0; i < length; ++i) {
    tuple->SetInternal(i, *elements->Get(i));
  }
  return scope.Escape(tuple);
}

Handle<PyString> Factory::NewRawStringLike(Tagged<Klass> klass_self,
                                           int64_t str_length,
                                           bool in_meta_space,
                                           bool allocate_properties_dict) {
  size_t object_size = PyString::ComputeObjectSize(str_length);
  Tagged<PyString> object(AllocateRaw(
      object_size, in_meta_space ? Heap::AllocationSpace::kMetaSpace
                                 : Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    object->length_ = str_length;
    object->hash_ = 0;
    object->writable_buffer()[str_length] = '\0';
    PyObject::SetKlass(object, klass_self);
    PyObject::SetProperties(object, Tagged<PyDict>::null());
  }

  if (allocate_properties_dict) {
    EscapableHandleScope scope;
    Handle<PyString> str_handle(object);
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*str_handle, *properties);
    return scope.Escape(str_handle);
  }

  return handle(object);
}

Handle<PyString> Factory::NewRawString(int64_t str_length, bool in_meta_space) {
  EscapableHandleScope scope;
  return scope.Escape(NewRawStringLike(PyStringKlass::GetInstance(), str_length,
                                       in_meta_space, false));
}

Handle<PyString> Factory::NewString(const char* source,
                                    int64_t str_length,
                                    bool in_meta_space) {
  EscapableHandleScope scope;
  Handle<PyString> object = NewRawString(str_length, in_meta_space);
  std::memcpy(object->writable_buffer(), source, str_length);
  return scope.Escape(object);
}

Handle<PyString> Factory::NewConsString(Handle<PyString> left,
                                        Handle<PyString> right) {
  EscapableHandleScope scope;

  auto new_length = left->length() + right->length();
  Handle<PyString> new_object =
      NewRawStringLike(PyStringKlass::GetInstance(), new_length, false, false);

  std::memcpy(new_object->writable_buffer(), left->buffer(), left->length());
  std::memcpy(new_object->writable_buffer() + left->length(), right->buffer(),
              right->length());

  return scope.Escape(new_object);
}

Handle<PyCodeObject> Factory::NewPyCodeObject() {
  EscapableHandleScope scope;

  auto klass = PyCodeObjectKlass::GetInstance();
  Handle<PyCodeObject> object(
      Allocate<PyCodeObject>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
    object->bytecodes_ = Tagged<PyObject>::null();
    object->consts_ = Tagged<PyObject>::null();
    object->localsplusnames_ = Tagged<PyObject>::null();
    object->localspluskinds_ = Tagged<PyObject>::null();
    object->names_ = Tagged<PyObject>::null();
    object->var_names_ = Tagged<PyObject>::null();
    object->free_vars_ = Tagged<PyObject>::null();
    object->cell_vars_ = Tagged<PyObject>::null();
    object->file_name_ = Tagged<PyObject>::null();
    object->co_name_ = Tagged<PyObject>::null();
    object->qual_name_ = Tagged<PyObject>::null();
    object->line_table_ = Tagged<PyObject>::null();
    object->exception_table_ = Tagged<PyObject>::null();
    object->no_table_ = Tagged<PyObject>::null();
    object->arg_count_ = 0;
    object->posonly_arg_count_ = 0;
    object->kwonly_arg_count_ = 0;
    object->stack_size_ = 0;
    object->flags_ = 0;
    object->nlocalsplus_ = 0;
    object->nlocals_ = 0;
    object->ncellvars_ = 0;
    object->nfreevars_ = 0;
    object->line_no_ = 0;
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

MaybeHandle<PyObject> Factory::NewPythonObject(
    Handle<PyTypeObject> type_object) {
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

  // 建立object实例与type object和klass之间的绑定关系
  PyObject::SetKlass(object, type_object->own_klass());
  RETURN_ON_EXCEPTION(isolate_, PyDict::Put(PyObject::GetProperties(object),
                                            ST(class), type_object));

  return scope.Escape(object);
}

Handle<PyListIterator> Factory::NewPyListIterator(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyListIteratorKlass::GetInstance();
  Handle<PyListIterator> iterator(
      Allocate<PyListIterator>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    iterator->iter_cnt_ = 0;
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyTupleIterator> Factory::NewPyTupleIterator(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyTupleIteratorKlass::GetInstance();
  Handle<PyTupleIterator> iterator(
      Allocate<PyTupleIterator>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    iterator->iter_cnt_ = 0;
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictKeys> Factory::NewPyDictKeys(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictKeysKlass::GetInstance();
  Handle<PyDictKeys> view(
      Allocate<PyDictKeys>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    view->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictValues> Factory::NewPyDictValues(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictValuesKlass::GetInstance();
  Handle<PyDictValues> view(
      Allocate<PyDictValues>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    view->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictItems> Factory::NewPyDictItems(Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictItemsKlass::GetInstance();
  Handle<PyDictItems> view(
      Allocate<PyDictItems>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    view->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictKeyIterator> Factory::NewPyDictKeyIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictKeyIteratorKlass::GetInstance();
  Handle<PyDictKeyIterator> iterator(
      Allocate<PyDictKeyIterator>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    iterator->iter_index_ = 0;
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictValueIterator> Factory::NewPyDictValueIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictValueIteratorKlass::GetInstance();
  Handle<PyDictValueIterator> iterator(
      Allocate<PyDictValueIterator>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    iterator->iter_index_ = 0;
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictItemIterator> Factory::NewPyDictItemIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope;

  auto klass = PyDictItemIteratorKlass::GetInstance();
  Handle<PyDictItemIterator> iterator(
      Allocate<PyDictItemIterator>(Heap::AllocationSpace::kNewSpace));
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->owner_ = owner.is_null() ? Tagged<PyObject>::null() : *owner;
    iterator->iter_index_ = 0;
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
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
