// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-iterator-klass.h"

#include <cassert>
#include <cstdio>

#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list.h"
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
  auto iterator = Handle<PyListIterator>::cast(self);
  auto list = iterator->owner();
  assert(!list.is_null());

  auto iter_cnt = iterator->iter_cnt();
  if (iter_cnt < list->length()) {
    auto result = list->Get(iter_cnt);
    iterator->increase_iter_cnt();
    return result.EscapeFrom(&scope);
  }
  return Handle<PyObject>::null();
}

Handle<PyObject> Native_Next(Handle<PyObject> self,
                             Handle<PyTuple> args,
                             Handle<PyDict> kwargs) {
  HandleScope scope;
  return NextImpl(self).EscapeFrom(&scope);
}
}  // namespace

Tagged<PyListIteratorKlass> PyListIteratorKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyListIteratorKlass> instance = isolate->py_list_iterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_iterator_klass(instance);
  }
  return instance;
}

void PyListIteratorKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.print = &Virtual_Print;
  vtable_.iter = &Virtual_Iter;
  vtable_.next = &Virtual_Next;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyListIteratorKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置类属性
  auto klass_properties = PyDict::NewInstance();

  PyDict::Put(klass_properties, ST(next),
              PyFunction::NewInstance(&Native_Next, ST(next)));

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("list_iterator"));
}

void PyListIteratorKlass::Finalize() {
  Isolate::Current()->set_py_list_iterator_klass(
      Tagged<PyListIteratorKlass>::null());
}

void PyListIteratorKlass::Virtual_Print(Handle<PyObject> self) {
  std::printf("<list_iterator object at 0x%p>",
              reinterpret_cast<void*>((*self).ptr()));
}

Handle<PyObject> PyListIteratorKlass::Virtual_Iter(Handle<PyObject> self) {
  return self;
}

Handle<PyObject> PyListIteratorKlass::Virtual_Next(Handle<PyObject> self) {
  return NextImpl(self);
}

size_t PyListIteratorKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyListIterator));
}

void PyListIteratorKlass::Virtual_Iterate(Tagged<PyObject> self,
                                          ObjectVisitor* v) {
  assert(IsPyListIterator(self));
  auto iterator = Tagged<PyListIterator>::cast(self);
  v->VisitPointer(&iterator->owner_);
}

}  // namespace saauso::internal
