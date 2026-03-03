// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-klass.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <vector>

#include "src/builtins/builtins-py-list-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-iterator.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"
#include "src/utils/stable-merge-sort.h"
#include "src/utils/utils.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static
Tagged<PyListKlass> PyListKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyListKlass> instance = isolate->py_list_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  // 设置内建布局字段
  set_native_layout_kind(NativeLayoutKind::kList);
  set_native_layout_base(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.construct_instance = &Virtual_ConstructInstance;
  vtable_.len = &Virtual_Len;
  vtable_.print = &Virtual_Print;
  vtable_.add = &Virtual_Add;
  vtable_.mul = &Virtual_Mul;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DelSubscr;
  vtable_.less = &Virtual_Less;
  vtable_.iter = &Virtual_Iter;
  vtable_.contains = &Virtual_Contains;
  vtable_.equal = &Virtual_Equal;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyListKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 设置类属性
  auto klass_properties = PyDict::NewInstance();

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate,
                      PyListBuiltinMethods::Install(isolate, klass_properties));

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("list"));

  return JustVoid();
}

void PyListKlass::Finalize() {
  Isolate::Current()->set_py_list_klass(Tagged<PyListKlass>::null());
}

MaybeHandle<PyObject> PyListKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self->native_layout_kind() == NativeLayoutKind::kList);
  auto* isolate = Isolate::Current();
  bool is_exact_list = klass_self == PyListKlass::GetInstance();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (is_exact_list) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "list() takes no keyword arguments\n");
      return kNullMaybeHandle;
    }
    if (argc > 1) {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "list expected at most 1 argument, got %" PRId64,
                          argc);
      return kNullMaybeHandle;
    }
  }

  Handle<PyList> result = isolate->factory()->NewPyListLike(
      klass_self, PyList::kMinimumCapacity, !is_exact_list);
  if (!is_exact_list) {
    auto properties = PyObject::GetProperties(result);
    RETURN_ON_EXCEPTION(
        isolate, PyDict::Put(properties, ST(class), klass_self->type_object()));
  }

  Handle<PyObject> init_method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, result, ST(init), init_method));

  if (init_method.is_null()) {
    if (!is_exact_list) {
      if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "list() takes no keyword arguments\n");
        return kNullMaybeHandle;
      }
      if (argc > 1) {
        Runtime_ThrowErrorf(ExceptionType::kTypeError,
                            "list expected at most 1 argument, got %" PRId64,
                            argc);
        return kNullMaybeHandle;
      }
    }
    if (argc == 1) {
      Handle<PyObject> extend_result;
      if (!Runtime_ExtendListByItratableObject(result, pos_args->Get(0))
               .ToHandle(&extend_result)) {
        return kNullMaybeHandle;
      }
    }
    return result;
  }

  if (!init_method.is_null()) {
    if (Execution::Call(isolate, init_method, result, pos_args,
                        Handle<PyDict>::cast(kwargs))
            .IsEmpty()) {
      return kNullMaybeHandle;
    }
  }
  return result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(PySmi::FromInt(PyList::CastListLike(self)->length()));
}

MaybeHandle<PyObject> PyListKlass::Virtual_Print(Handle<PyObject> self) {
  auto list = PyList::CastListLike(self);

  std::printf("[");

  for (auto i = 0; i < list->length(); ++i) {
    if (i > 0) {
      std::printf(", ");
    }
    if (PyObject::Print(list->Get(i)).IsEmpty()) {
      return kNullMaybeHandle;
    }
  }

  std::printf("]");
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> PyListKlass::Virtual_Add(Handle<PyObject> self,
                                               Handle<PyObject> other) {
  auto list1 = PyList::CastListLike(self);
  if (!PyList::IsListLike(other)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "can only concatenate list (not \"%s\") to list",
                        PyObject::GetKlass(other)->name()->buffer());
    return kNullMaybeHandle;
  }
  auto list2 = PyList::CastListLike(other);

  auto new_result = PyList::NewInstance(list1->length() + list2->length());
  for (auto i = 0; i < list1->length(); ++i) {
    PyList::Append(new_result, list1->Get(i));
  }

  for (auto i = 0; i < list2->length(); ++i) {
    PyList::Append(new_result, list2->Get(i));
  }

  return new_result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Mul(Handle<PyObject> self,
                                               Handle<PyObject> coeff) {
  if (!IsPySmi(coeff)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "can't multiply sequence by non-int of type '%s'",
                        PyObject::GetKlass(coeff)->name()->buffer());
    return kNullMaybeHandle;
  }

  auto list = PyList::CastListLike(self);
  auto decoded_coeff = std::max(static_cast<int64_t>(0),
                                PySmi::ToInt(Handle<PySmi>::cast(coeff)));

  auto result = PyList::NewInstance(list->length() * decoded_coeff);
  while (decoded_coeff-- > 0) {
    for (int i = 0; i < list->length(); ++i) {
      PyList::Append(result, list->Get(i));
    }
  }

  return result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Subscr(Handle<PyObject> self,
                                                  Handle<PyObject> subscr) {
  if (!IsPySmi(subscr)) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "list indices must be integers or slices, not %s",
                        PyObject::GetKlass(subscr)->name()->buffer());
    return kNullMaybeHandle;
  }

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  return PyList::CastListLike(self)->Get(decoded_subscr);
}

MaybeHandle<PyObject> PyListKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                                       Handle<PyObject> subscr,
                                                       Handle<PyObject> value) {
  auto list = PyList::CastListLike(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    Runtime_ThrowError(ExceptionType::kIndexError,
                       "list assignment index out of range");
    return kNullMaybeHandle;
  }

  list->Set(decoded_subscr, value);
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> PyListKlass::Virtual_DelSubscr(Handle<PyObject> self,
                                                     Handle<PyObject> subscr) {
  auto list = PyList::CastListLike(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    Runtime_ThrowError(ExceptionType::kIndexError,
                       "list assignment index out of range");
    return kNullMaybeHandle;
  }

  list->RemoveByIndex(decoded_subscr);
  return handle(Isolate::Current()->py_none_object());
}

Maybe<bool> PyListKlass::Virtual_Less(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  auto list_l = PyList::CastListLike(self);
  if (!PyList::IsListLike(other)) {
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "'<' not supported between instances of 'list' and '%s'",
        PyObject::GetKlass(other)->name()->buffer());
    return kNullMaybe;
  }
  auto list_r = PyList::CastListLike(other);
  auto min_len = std::min(list_l->length(), list_r->length());

  for (auto i = 0; i < min_len; ++i) {
    auto l = list_l->Get(i);
    auto r = list_r->Get(i);

    Maybe<bool> less_mb = PyObject::LessBool(l, r);
    if (less_mb.IsNothing()) {
      return kNullMaybe;
    }
    if (less_mb.ToChecked()) {
      return Maybe<bool>(true);
    }

    Maybe<bool> greater_mb = PyObject::GreaterBool(l, r);
    if (greater_mb.IsNothing()) {
      return kNullMaybe;
    }
    if (greater_mb.ToChecked()) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(list_l->length() < list_r->length());
}

Maybe<bool> PyListKlass::Virtual_Contains(Handle<PyObject> self,
                                          Handle<PyObject> target) {
  auto list = PyList::CastListLike(self);
  for (auto i = 0; i < list->length(); ++i) {
    Maybe<bool> eq = PyObject::EqualBool(list->Get(i), target);
    if (eq.IsNothing()) {
      return kNullMaybe;
    }
    if (eq.ToChecked()) {
      return Maybe<bool>(true);
    }
  }

  return Maybe<bool>(false);
}

Maybe<bool> PyListKlass::Virtual_Equal(Handle<PyObject> self,
                                       Handle<PyObject> target) {
  auto list1 = PyList::CastListLike(self);
  if (!PyList::IsListLike(target)) {
    return Maybe<bool>(false);
  }
  auto list2 = PyList::CastListLike(target);

  if (list1->length() != list2->length()) {
    return Maybe<bool>(false);
  }

  for (auto i = 0; i < list1->length(); ++i) {
    Maybe<bool> eq = PyObject::EqualBool(list1->Get(i), list2->Get(i));
    if (eq.IsNothing()) {
      return kNullMaybe;
    }
    if (!eq.ToChecked()) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(true);
}

MaybeHandle<PyObject> PyListKlass::Virtual_Iter(Handle<PyObject> object) {
  return PyListIterator::NewInstance(PyList::CastListLike(object));
}

size_t PyListKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyList));
}

void PyListKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(PyList::IsListLike(self));
  v->VisitPointer(&Tagged<PyList>::cast(self)->array_);
}

}  // namespace saauso::internal
