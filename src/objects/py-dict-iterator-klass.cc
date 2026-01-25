// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-iterator-klass.h"

#include <cassert>
#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-dict-iterator.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/isolate.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NextImpl(Handle<PyObject> self) {
  HandleScope scope;
  auto iterator = Handle<PyDictIterator>::cast(self);
  auto dict = iterator->owner();
  assert(!dict.IsNull());

  int64_t index = iterator->iter_index();
  for (; index < dict->capacity(); ++index) {
    Handle<PyObject> result = dict->KeyAtIndex(index);
    if (result.IsNull()) {
      continue;
    }
    iterator->set_iter_index(index + 1);
    return result.EscapeFrom(&scope);
  }

  iterator->set_iter_index(dict->capacity());
  return Handle<PyObject>::Null();
}

}  // namespace

Handle<PyObject> PyDictIteratorKlass::Native_Next(Handle<PyObject> self,
                                                  Handle<PyTuple> args,
                                                  Handle<PyDict> kwargs) {
  HandleScope scope;
  return NextImpl(self).EscapeFrom(&scope);
}

Tagged<PyDictIteratorKlass> PyDictIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictIteratorKlass> instance = isolate->py_dict_iterator_klass();
  if (instance.IsNull()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_iterator_klass(instance);
  }
  return instance;
}

void PyDictIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(this);

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyDictIteratorKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  auto klass_properties = PyDict::NewInstance();
  PyDict::Put(klass_properties, ST(next),
              PyFunction::NewInstance(&Native_Next, ST(next)));
  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("dict_keyiterator"));
}

void PyDictIteratorKlass::Finalize() {
  Isolate::Current()->set_py_dict_iterator_klass(
      Tagged<PyDictIteratorKlass>::Null());
}

void PyDictIteratorKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("<dict_keyiterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> PyDictIteratorKlass::Virtual_Iter(Handle<PyObject> self) {
  return self;
}

Handle<PyObject> PyDictIteratorKlass::Virtual_Next(Handle<PyObject> self) {
  return NextImpl(self);
}

size_t PyDictIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDictIterator));
}

void PyDictIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                          ObjectVisitor* v) {
  assert(IsPyDictIterator(self));
  auto iterator = Tagged<PyDictIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

}  // namespace saauso::internal
