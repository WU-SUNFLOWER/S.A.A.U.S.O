// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-iterator-klass.h"

#include <cassert>
#include <cstdio>

#include "src/builtins/builtins-py-tuple-iterator-methods.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NextImpl(Handle<PyObject> self) {
  EscapableHandleScope scope;

  auto iterator = Handle<PyTupleIterator>::cast(self);
  auto tuple = iterator->owner();
  assert(!tuple.is_null());

  auto result = Handle<PyObject>::null();
  auto iter_cnt = iterator->iter_cnt();
  if (iter_cnt < tuple->length()) {
    result = tuple->Get(iter_cnt);
    iterator->increase_iter_cnt();
  }

  return scope.Escape(result);
}

}  // namespace

Tagged<PyTupleIteratorKlass> PyTupleIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyTupleIteratorKlass> instance = isolate->py_tuple_iterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTupleIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_tuple_iterator_klass(instance);
  }
  return instance;
}

void PyTupleIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyTupleIteratorKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  auto klass_properties = PyDict::NewInstance();
  RETURN_ON_EXCEPTION(isolate, PyTupleIteratorBuiltinMethods::Install(
                                   isolate, klass_properties));
  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  set_name(PyString::NewInstance("tuple_iterator"));

  return JustVoid();
}

void PyTupleIteratorKlass::Finalize() {
  Isolate::Current()->set_py_tuple_iterator_klass(
      Tagged<PyTupleIteratorKlass>::null());
}

MaybeHandle<PyObject> PyTupleIteratorKlass::Virtual_Print(
    Handle<PyObject> self) {
  std::printf("<tuple_iterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> PyTupleIteratorKlass::Virtual_Iter(
    Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyTupleIteratorKlass::Virtual_Next(
    Handle<PyObject> self) {
  Handle<PyObject> result = NextImpl(self);
  if (result.is_null()) {
    Runtime_ThrowError(ExceptionType::kStopIteration);
    return kNullMaybeHandle;
  }
  return result;
}

size_t PyTupleIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyTupleIterator));
}

void PyTupleIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                           ObjectVisitor* v) {
  assert(IsPyTupleIterator(self));
  auto iterator = Tagged<PyTupleIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

}  // namespace saauso::internal
