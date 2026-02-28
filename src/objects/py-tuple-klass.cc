// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-klass.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "src/builtins/builtins-py-tuple-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-iterator.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/utils/maybe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PyTupleKlass> PyTupleKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyTupleKlass> instance = isolate->py_tuple_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTupleKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_tuple_klass(instance);
  }
  return instance;
}

void PyTupleKlass::PreInitialize() {
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.construct_instance = &Virtual_ConstructInstance;
  vtable_.len = &Virtual_Len;
  vtable_.print = &Virtual_Print;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DelSubscr;
  vtable_.contains = &Virtual_Contains;
  vtable_.equal = &Virtual_Equal;
  vtable_.iter = &Virtual_Iter;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyTupleKlass::Initialize() {
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  auto klass_properties = PyDict::NewInstance();

  // 安装内建方法
  PyTupleBuiltinMethods::Install(klass_properties);

  set_klass_properties(klass_properties);

  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  set_name(PyString::NewInstance("tuple"));
}

void PyTupleKlass::Finalize() {
  Isolate::Current()->set_py_tuple_klass(Tagged<PyTupleKlass>::null());
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyTupleKlass::GetInstance());

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "tuple() takes no keyword arguments\n");
    return kNullMaybeHandle;
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (argc == 0) {
    return PyTuple::NewInstance(0);
  }
  if (argc > 1) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "tuple expected at most 1 argument, got %" PRId64 "\n",
                        argc);
    return kNullMaybeHandle;
  }

  // tuple(tuple_obj) 直接返回自身（不可变对象的语义对齐 CPython）。
  Handle<PyObject> iterable = pos_args->Get(0);
  if (IsPyTuple(iterable)) {
    return iterable;
  }

  Handle<PyTuple> result;
  if (!Runtime_UnpackIterableObjectToTuple(iterable).ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return result;
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyTuple>::cast(self)->length()));
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Print(Handle<PyObject> self) {
  auto* isolate [[maybe_unused]] = Isolate::Current();
  auto tuple = Handle<PyTuple>::cast(self);
  std::printf("(");
  for (auto i = 0; i < tuple->length(); ++i) {
    if (i > 0) {
      std::printf(", ");
    }
    RETURN_ON_EXCEPTION(isolate, PyObject::Print(tuple->Get(i)));
  }
  if (tuple->length() == 1) {
    std::printf(",");
  }
  std::printf(")");
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Subscr(Handle<PyObject> self,
                                                   Handle<PyObject> subscr) {
  auto tuple = Handle<PyTuple>::cast(self);
  if (!IsPySmi(*subscr)) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "tuple indices must be integers\n");
    return kNullMaybeHandle;
  }

  auto index = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(index, static_cast<int64_t>(0), tuple->length())) {
    Runtime_ThrowError(ExceptionType::kIndexError,
                       "tuple index out of range\n");
    return kNullMaybeHandle;
  }
  return tuple->Get(index);
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_StoreSubscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr,
    Handle<PyObject> value) {
  Runtime_ThrowError(ExceptionType::kTypeError,
                     "'tuple' object does not support item assignment\n");
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                                      Handle<PyObject> subscr) {
  Runtime_ThrowError(ExceptionType::kTypeError,
                     "'tuple' object doesn't support item deletion\n");
  return kNullMaybeHandle;
}

Maybe<bool> PyTupleKlass::Virtual_Contains(Handle<PyObject> self,
                                           Handle<PyObject> target) {
  auto* isolate [[maybe_unused]] = Isolate::Current();
  auto tuple = Handle<PyTuple>::cast(self);
  for (auto i = 0; i < tuple->length(); ++i) {
    bool eq;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, eq,
                               PyObject::EqualBool(tuple->Get(i), target));
    if (eq) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

Maybe<bool> PyTupleKlass::Virtual_Equal(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  if (!IsPyTuple(*other)) {
    return Maybe<bool>(false);
  }

  auto* isolate [[maybe_unused]] = Isolate::Current();

  auto tuple1 = Handle<PyTuple>::cast(self);
  auto tuple2 = Handle<PyTuple>::cast(other);

  if (tuple1->length() != tuple2->length()) {
    return Maybe<bool>(false);
  }

  for (auto i = 0; i < tuple1->length(); ++i) {
    bool eq;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, eq, PyObject::EqualBool(tuple1->Get(i), tuple2->Get(i)));
    if (!eq) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(true);
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Iter(Handle<PyObject> self) {
  return PyTupleIterator::NewInstance(Handle<PyTuple>::cast(self));
}

size_t PyTupleKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(IsPyTuple(self));
  return PyTuple::ComputeObjectSize(Tagged<PyTuple>::cast(self)->length());
}

void PyTupleKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyTuple(self));
  auto tuple = Tagged<PyTuple>::cast(self);
  v->VisitPointers(tuple->data(), tuple->data() + tuple->length());
}

}  // namespace saauso::internal
