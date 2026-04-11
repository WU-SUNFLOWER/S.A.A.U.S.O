// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/heap/factory.h"

#include <algorithm>
#include <cstring>

#include "include/saauso-internal.h"
#include "src/execution/exception-roots.h"
#include "src/execution/exception-utils.h"
#include "src/handles/handle-scopes.h"
#include "src/objects/cell-klass.h"
#include "src/objects/cell.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/fixed-array.h"
#include "src/objects/klass.h"
#include "src/objects/py-base-exception.h"
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
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/objects/templates.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Tagged<Klass> Factory::NewPythonKlass() {
  auto klass = Allocate<Klass>(Heap::AllocationSpace::kMetaSpace);
  {
    DisallowHeapAllocation disallow(isolate_);
    klass->set_vtable({});
    klass->set_klass_properties(Handle<PyDict>::null());
    klass->set_name(Handle<PyString>::null());
    klass->set_type_object(Handle<PyTypeObject>::null());
    klass->set_supers(Handle<PyList>::null());
    klass->set_mro(Handle<PyList>::null());
    klass->set_native_layout_base(Tagged<Klass>::null());
    klass->set_native_layout_kind(NativeLayoutKind::kPyObject);
    klass->set_instance_has_properties_dict(false);
  }
  return klass;
}

Handle<PyDict> Factory::NewPyDict(int64_t init_capacity) {
  return NewDictLike(PyDictKlass::GetInstance(isolate_), init_capacity);
}

Handle<PyDict> Factory::NewDictLike(Tagged<Klass> klass_self,
                                    int64_t init_capacity) {
  assert((init_capacity & (init_capacity - 1)) == 0);

  EscapableHandleScope scope(isolate_);
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace),
                        isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_occupied(0);
    PyDict::SetKlass(object, klass_self);
    object->set_data(Tagged<FixedArray>::null());
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<FixedArray> data = NewFixedArray(init_capacity * 2);
  object->set_data(data);

  if (klass_self->instance_has_properties_dict()) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  return scope.Escape(object);
}

Handle<PyDict> Factory::NewPyDictWithoutAllocateData() {
  auto klass = PyDictKlass::GetInstance(isolate_);
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace),
                        isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_occupied(0);
    PyDict::SetKlass(object, klass);
    object->set_data(Tagged<FixedArray>::null());
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  return object;
}

Handle<PyDict> Factory::CopyPyDict(Handle<PyDict> dict) {
  EscapableHandleScope scope(isolate_);

  Handle<PyDict> result = NewPyDictWithoutAllocateData();
  Handle<FixedArray> data = CopyFixedArray(dict->data(isolate_));
  result->set_occupied(dict->occupied());
  result->set_data(data);

  return scope.Escape(result);
}

Handle<FixedArray> Factory::NewFixedArray(int64_t capacity) {
  assert(0 <= capacity);

  EscapableHandleScope scope(isolate_);

  size_t object_size = FixedArray::ComputeObjectSize(capacity);
  auto klass = FixedArrayKlass::GetInstance(isolate_);
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
  return scope.Escape(handle(object, isolate_));
}

Handle<FixedArray> Factory::CopyFixedArrayAndGrow(Handle<FixedArray> array,
                                                  int64_t grow_by) {
  // 新创建的fixed array的容量不能比拷贝对象还小
  assert(0 <= grow_by);

  int64_t old_capacity = array->capacity();
  int64_t new_capacity = old_capacity + grow_by;
  Handle<FixedArray> result = NewFixedArray(new_capacity);

  {
    DisallowHeapAllocation disallow(isolate_);
    // 拷贝旧数据
    std::memcpy(result->data(), array->data(),
                old_capacity * sizeof(Tagged<PyObject>));
  }

  return result;
}

Handle<FixedArray> Factory::CopyFixedArray(Handle<FixedArray> array) {
  return CopyFixedArrayAndGrow(array, 0);
}

Handle<Cell> Factory::NewCell() {
  Handle<Cell> object(Allocate<Cell>(Heap::AllocationSpace::kNewSpace),
                      isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
    object->set_value(Tagged<PyObject>(kNullAddress));
    PyObject::SetKlass(object, CellKlass::GetInstance(isolate_));
  }
  return object;
}

Handle<PySmi> Factory::NewSmiFromInt(int64_t value) {
  return handle(PySmi::FromInt(value), isolate_);
}

Handle<PyFloat> Factory::NewPyFloat(double value) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyFloatKlass::GetInstance(isolate_);
  Handle<PyFloat> object(Allocate<PyFloat>(Heap::AllocationSpace::kNewSpace),
                         isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_value(value);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  return scope.Escape(object);
}

Handle<PyList> Factory::NewPyListLike(Tagged<Klass> klass_self,
                                      int64_t init_capacity) {
  EscapableHandleScope scope(isolate_);

  Handle<PyList> object(Allocate<PyList>(Heap::AllocationSpace::kNewSpace),
                        isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_length(0);
    object->set_array(Tagged<FixedArray>::null());
    PyObject::SetKlass(object, klass_self);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<FixedArray> array =
      NewFixedArray(std::max(PyList::kMinimumCapacity, init_capacity));
  object->set_array(array);

  if (klass_self->instance_has_properties_dict()) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  return scope.Escape(object);
}

Handle<PyList> Factory::NewPyList(int64_t init_capacity) {
  EscapableHandleScope scope(isolate_);
  return scope.Escape(
      NewPyListLike(PyListKlass::GetInstance(isolate_), init_capacity));
}

Handle<PyTuple> Factory::NewPyTupleLike(Tagged<Klass> klass_self,
                                        int64_t length) {
  assert(0 <= length);

  EscapableHandleScope scope(isolate_);

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

  Handle<PyTuple> object_handle = handle(object, isolate_);
  if (klass_self->instance_has_properties_dict()) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object_handle, *properties);
  }

  return scope.Escape(object_handle);
}

Handle<PyTuple> Factory::NewPyTuple(int64_t length) {
  return NewPyTupleLike(PyTupleKlass::GetInstance(isolate_), length);
}

Handle<PyTuple> Factory::NewPyTupleWithElements(Handle<PyList> elements) {
  EscapableHandleScope scope(isolate_);
  auto length = elements->length();
  auto tuple = NewPyTuple(length);
  for (auto i = 0; i < length; ++i) {
    tuple->SetInternal(i, *elements->Get(i, isolate_));
  }
  return scope.Escape(tuple);
}

Handle<PyString> Factory::NewRawStringLike(Tagged<Klass> klass_self,
                                           int64_t str_length,
                                           bool in_meta_space) {
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

  if (klass_self->instance_has_properties_dict()) {
    EscapableHandleScope scope(isolate_);
    Handle<PyString> str_handle(object, isolate_);
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*str_handle, *properties);
    return scope.Escape(str_handle);
  }

  return handle(object, isolate_);
}

Handle<PyString> Factory::NewRawString(int64_t str_length, bool in_meta_space) {
  EscapableHandleScope scope(isolate_);
  return scope.Escape(NewRawStringLike(PyStringKlass::GetInstance(isolate_),
                                       str_length, in_meta_space));
}

Handle<PyString> Factory::NewString(const char* source,
                                    int64_t str_length,
                                    bool in_meta_space) {
  EscapableHandleScope scope(isolate_);
  Handle<PyString> object = NewRawString(str_length, in_meta_space);
  std::memcpy(object->writable_buffer(), source, str_length);
  return scope.Escape(object);
}

Handle<PyString> Factory::NewStringLike(Tagged<Klass> klass_self,
                                        Handle<PyString> source) {
  EscapableHandleScope scope(isolate_);
  int64_t length = source->length();
  Handle<PyString> object = NewRawStringLike(klass_self, length, false);
  std::memcpy(object->writable_buffer(), source->buffer(), length);
  return scope.Escape(object);
}

Handle<PyString> Factory::NewCopiedSubstring(Handle<PyString> str,
                                             int64_t begin,
                                             int64_t length) {
  EscapableHandleScope scope(isolate_);

  assert(begin + length <= str->length());

  Handle<PyString> object = NewRawString(length, false);
  std::memcpy(object->writable_buffer(), str->buffer() + begin, length);

  return scope.Escape(object);
}

Handle<PyString> Factory::NewConsString(Handle<PyString> left,
                                        Handle<PyString> right) {
  EscapableHandleScope scope(isolate_);

  auto new_length = left->length() + right->length();
  Handle<PyString> new_object =
      NewRawStringLike(PyStringKlass::GetInstance(isolate_), new_length, false);

  std::memcpy(new_object->writable_buffer(), left->buffer(), left->length());
  std::memcpy(new_object->writable_buffer() + left->length(), right->buffer(),
              right->length());

  return scope.Escape(new_object);
}

Handle<PyCodeObject> Factory::NewPyCodeObject() {
  EscapableHandleScope scope(isolate_);

  auto klass = PyCodeObjectKlass::GetInstance(isolate_);
  Handle<PyCodeObject> object(
      Allocate<PyCodeObject>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
    object->set_bytecodes(Tagged<PyString>::null());
    object->set_consts(Tagged<PyTuple>::null());
    object->set_localsplusnames(Tagged<PyTuple>::null());
    object->set_localspluskinds(Tagged<PyString>::null());
    object->set_names(Tagged<PyTuple>::null());
    object->set_var_names(Tagged<PyTuple>::null());
    object->set_free_vars(Tagged<PyTuple>::null());
    object->set_cell_vars(Tagged<PyTuple>::null());
    object->set_file_name(Tagged<PyString>::null());
    object->set_co_name(Tagged<PyString>::null());
    object->set_qual_name(Tagged<PyString>::null());
    object->set_line_table(Tagged<PyString>::null());
    object->set_exception_table(Tagged<PyString>::null());
    object->set_no_table(Tagged<PyString>::null());
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
  EscapableHandleScope scope(isolate_);

  auto klass = PyFunctionKlass::GetInstance(isolate_);
  Handle<PyFunction> object(
      Allocate<PyFunction>(Heap::AllocationSpace::kNewSpace), isolate_);

  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_func_code(Tagged<PyCodeObject>::null());
    object->set_func_name(Tagged<PyString>::null());
    object->set_func_globals(Tagged<PyDict>::null());
    object->set_default_args(Tagged<PyTuple>::null());
    object->set_closures(Tagged<PyTuple>::null());
    object->set_native_func(nullptr);
    object->set_native_func_with_closure(nullptr);
    object->set_native_closure_data(Tagged<PyObject>::null());
    object->set_native_access_flag(NativeFuncAccessFlag::kStatic);
    object->set_native_owner_type(Tagged<PyTypeObject>::null());
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  PyObject::SetProperties(*object, *properties);

  return scope.Escape(object);
}

Handle<PyFunction> Factory::NewPyFunctionWithCodeObject(
    Handle<PyCodeObject> code_object) {
  EscapableHandleScope scope(isolate_);

  Handle<PyFunction> object = NewPyFunction();

  object->set_func_code(code_object);
  object->set_func_name(code_object->co_name(isolate_));

  // 绑定klass
  PyObject::SetKlass(object, PyFunctionKlass::GetInstance(isolate_));

  return scope.Escape(object);
}

Handle<PyFunction> Factory::NewPyFunctionWithTemplate(
    const FunctionTemplateInfo& func_template) {
  EscapableHandleScope scope(isolate_);

  Handle<PyFunction> object = NewPyFunction();

  assert(func_template.function() != nullptr ||
         func_template.function_with_closure() != nullptr);
  object->set_native_func(func_template.function());
  object->set_native_func_with_closure(func_template.function_with_closure());
  object->set_native_closure_data(func_template.closure_data().is_null()
                                      ? Tagged<PyObject>::null()
                                      : *func_template.closure_data());
  object->set_native_access_flag(func_template.native_access_flag());
  object->set_native_owner_type(func_template.native_owner_type().is_null()
                                    ? Tagged<PyTypeObject>::null()
                                    : *func_template.native_owner_type());

  assert(!func_template.name().is_null());
  object->set_func_name(*func_template.name());

  // 绑定klass
  PyObject::SetKlass(object, NativeFunctionKlass::GetInstance(isolate_));

  return scope.Escape(object);
}

Handle<MethodObject> Factory::NewMethodObject(Handle<PyObject> func,
                                              Handle<PyObject> owner) {
  auto klass = MethodObjectKlass::GetInstance(isolate_);
  Handle<MethodObject> object(
      Allocate<MethodObject>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    object->set_owner(Tagged<PyObject>::null());
    object->set_func(Tagged<PyObject>::null());
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }
  object->set_owner(owner);
  object->set_func(func);
  return object;
}

MaybeHandle<PyModule> Factory::NewPyModule() {
  EscapableHandleScope scope(isolate_);

  auto klass = PyModuleKlass::GetInstance(isolate_);
  Handle<PyModule> object(Allocate<PyModule>(Heap::AllocationSpace::kNewSpace),
                          isolate_);

  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  if (klass->instance_has_properties_dict()) [[likely]] {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  return scope.Escape(object);
}

MaybeHandle<PyTypeObject> Factory::NewPyTypeObject() {
  EscapableHandleScope scope(isolate_);

  auto klass = PyTypeObjectKlass::GetInstance(isolate_);
  Handle<PyTypeObject> object(
      Allocate<PyTypeObject>(Heap::AllocationSpace::kNewSpace), isolate_);

  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    object->set_own_klass(Tagged<Klass>::null());
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
  PyObject::SetProperties(*object, *properties);

  return scope.Escape(object);
}

Handle<PyBaseException> Factory::NewException(ExceptionType type,
                                              Handle<PyTuple> args) {
  EscapableHandleScope scope(isolate_);

  Handle<PyTypeObject> exception_type = isolate_->exception_roots()->Get(type);
  Tagged<Klass> exception_klass = exception_type->own_klass();

  assert(exception_klass->native_layout_kind() ==
         NativeLayoutKind::kBaseException);

  Handle<PyTuple> init_args = args;
  if (init_args.is_null()) {
    init_args = NewPyTuple(0);
  }

  Handle<PyBaseException> exception(
      Allocate<PyBaseException>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetProperties(*exception, Tagged<PyDict>::null());
    exception->set_args(init_args);
    PyObject::SetKlass(exception, exception_klass);
  }

  return scope.Escape(exception);
}

Handle<PyBaseException> Factory::NewExceptionFromMessage(
    ExceptionType type,
    Handle<PyString> message) {
  Handle<PyTuple> args = NewPyTuple(message.is_null() ? 0 : 1);
  if (!message.is_null()) {
    args->SetInternal(0, message);
  }
  return NewException(type, args);
}

MaybeHandle<PyObject> Factory::NewPythonObject(
    Handle<PyTypeObject> type_object) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyObjectKlass::GetInstance(isolate_);
  Handle<PyObject> object;
  // 目前Factory::NewPythonObject中加入了针对异常对象的特化逻辑。
  // 未来需要将异常对象的创建下沉为独立的 Factory::NewException API。
  if (type_object->own_klass()->native_layout_kind() ==
      NativeLayoutKind::kBaseException) {
    Handle<PyBaseException> base_exception(
        Allocate<PyBaseException>(Heap::AllocationSpace::kNewSpace), isolate_);
    {
      DisallowHeapAllocation disallow(isolate_);
      PyObject::SetKlass(base_exception, klass);
      PyObject::SetProperties(*base_exception, Tagged<PyDict>::null());
      base_exception->set_args(Tagged<PyTuple>::null());
    }
    object = Handle<PyObject>::cast(base_exception);
  } else {
    object = Handle<PyObject>(
        Allocate<PyObject>(Heap::AllocationSpace::kNewSpace), isolate_);
    {
      DisallowHeapAllocation disallow(isolate_);
      PyObject::SetKlass(object, klass);
      PyObject::SetProperties(*object, Tagged<PyDict>::null());
    }
  }

  if (type_object->own_klass()->instance_has_properties_dict()) {
    Handle<PyDict> properties = NewPyDict(PyDict::kMinimumCapacity);
    PyObject::SetProperties(*object, *properties);
  }

  // 建立object实例与type object和klass之间的绑定关系
  PyObject::SetKlass(object, type_object->own_klass());

  return scope.Escape(object);
}

Handle<PyListIterator> Factory::NewPyListIterator(Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyListIteratorKlass::GetInstance(isolate_);
  Handle<PyListIterator> iterator(
      Allocate<PyListIterator>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->set_owner(owner.is_null() ? Tagged<PyList>::null()
                                        : Tagged<PyList>::cast(*owner));
    iterator->set_iter_cnt(0);
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyTupleIterator> Factory::NewPyTupleIterator(Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyTupleIteratorKlass::GetInstance(isolate_);
  Handle<PyTupleIterator> iterator(
      Allocate<PyTupleIterator>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->set_owner(owner.is_null() ? Tagged<PyTuple>::null()
                                        : Tagged<PyTuple>::cast(*owner));
    iterator->set_iter_cnt(0);
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictKeys> Factory::NewPyDictKeys(Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictKeysKlass::GetInstance(isolate_);
  Handle<PyDictKeys> view(
      Allocate<PyDictKeys>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    view->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                    : Tagged<PyDict>::cast(*owner));
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictValues> Factory::NewPyDictValues(Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictValuesKlass::GetInstance(isolate_);
  Handle<PyDictValues> view(
      Allocate<PyDictValues>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    view->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                    : Tagged<PyDict>::cast(*owner));
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictItems> Factory::NewPyDictItems(Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictItemsKlass::GetInstance(isolate_);
  Handle<PyDictItems> view(
      Allocate<PyDictItems>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    view->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                    : Tagged<PyDict>::cast(*owner));
    PyObject::SetKlass(view, klass);
    PyObject::SetProperties(*view, Tagged<PyDict>::null());
  }
  return scope.Escape(view);
}

Handle<PyDictKeyIterator> Factory::NewPyDictKeyIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictKeyIteratorKlass::GetInstance(isolate_);
  Handle<PyDictKeyIterator> iterator(
      Allocate<PyDictKeyIterator>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                        : Tagged<PyDict>::cast(*owner));
    iterator->set_iter_index(0);
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictValueIterator> Factory::NewPyDictValueIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictValueIteratorKlass::GetInstance(isolate_);
  Handle<PyDictValueIterator> iterator(
      Allocate<PyDictValueIterator>(Heap::AllocationSpace::kNewSpace),
      isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                        : Tagged<PyDict>::cast(*owner));
    iterator->set_iter_index(0);
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyDictItemIterator> Factory::NewPyDictItemIterator(
    Handle<PyObject> owner) {
  EscapableHandleScope scope(isolate_);

  auto klass = PyDictItemIteratorKlass::GetInstance(isolate_);
  Handle<PyDictItemIterator> iterator(
      Allocate<PyDictItemIterator>(Heap::AllocationSpace::kNewSpace), isolate_);
  {
    DisallowHeapAllocation disallow(isolate_);
    iterator->set_owner(owner.is_null() ? Tagged<PyDict>::null()
                                        : Tagged<PyDict>::cast(*owner));
    iterator->set_iter_index(0);
    PyObject::SetKlass(iterator, klass);
    PyObject::SetProperties(*iterator, Tagged<PyDict>::null());
  }
  return scope.Escape(iterator);
}

Handle<PyBoolean> Factory::ToPyBoolean(bool condition) {
  return handle(
      condition ? isolate_->py_true_object() : isolate_->py_false_object(),
      isolate_);
}

Handle<PyBoolean> Factory::py_true_object() const {
  return handle(isolate_->py_true_object(), isolate_);
}

Handle<PyBoolean> Factory::py_false_object() const {
  return handle(isolate_->py_false_object(), isolate_);
}

Tagged<PyBoolean> Factory::NewPyBoolean(bool value) {
  auto klass = PyBooleanKlass::GetInstance(isolate_);
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
  auto klass = PyNoneKlass::GetInstance(isolate_);
  auto object = Allocate<PyNone>(Heap::AllocationSpace::kMetaSpace);
  {
    DisallowHeapAllocation disallow(isolate_);
    PyObject::SetKlass(object, klass);
    PyObject::SetProperties(object, Tagged<PyDict>::null());
  }
  return object;
}

Handle<PyNone> Factory::py_none_object() const {
  return handle(isolate_->py_none_object(), isolate_);
}

//////////////////////////////////////////////////////////////////////////////

Address Factory::AllocateRaw(size_t size_in_bytes,
                             Heap::AllocationSpace space) {
  return isolate_->heap()->AllocateRaw(size_in_bytes, space);
}

}  // namespace saauso::internal
