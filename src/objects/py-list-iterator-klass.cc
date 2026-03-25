// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-iterator-klass.h"

#include <cassert>
#include <cstdio>

#include "src/builtins/builtins-py-list-iterator-methods.h"
#include "src/execution/exception-types.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {
Handle<PyObject> NextImpl(Handle<PyObject> self) {
  EscapableHandleScope scope;

  auto iterator = Handle<PyListIterator>::cast(self);
  auto list = iterator->owner();
  assert(!list.is_null());

  auto iter_cnt = iterator->iter_cnt();
  auto result = Handle<PyObject>::null();
  if (iter_cnt < list->length()) {
    result = list->Get(iter_cnt);
    iterator->increase_iter_cnt();
  }

  return scope.Escape(result);
}
}  // namespace

Tagged<PyListIteratorKlass> PyListIteratorKlass::GetInstance(Isolate* isolate) {
  Tagged<PyListIteratorKlass> instance = isolate->py_list_iterator_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListIteratorKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_iterator_klass(instance);
  }
  return instance;
}

void PyListIteratorKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.iter_ = &Virtual_Iter;
  vtable_.next_ = &Virtual_Next;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyListIteratorKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate));
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 注入内建方法
  RETURN_ON_EXCEPTION(isolate, PyListIteratorBuiltinMethods::Install(
                                   isolate, klass_properties, type_object()));

  // 设置类名
  set_name(PyString::NewInstance("list_iterator"));

  return JustVoid();
}

void PyListIteratorKlass::Finalize(Isolate* isolate) {
  isolate->set_py_list_iterator_klass(Tagged<PyListIteratorKlass>::null());
}

MaybeHandle<PyObject> PyListIteratorKlass::Virtual_Iter(Isolate* isolate,
                                                        Handle<PyObject> self) {
  return self;
}

MaybeHandle<PyObject> PyListIteratorKlass::Virtual_Next(Isolate* isolate,
                                                        Handle<PyObject> self) {
  Handle<PyObject> result = NextImpl(self);
  if (result.is_null()) {
    Runtime_ThrowError(ExceptionType::kStopIteration);
    return kNullMaybeHandle;
  }
  return result;
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
