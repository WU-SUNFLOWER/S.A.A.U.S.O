// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-list-klass.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <vector>

#include "include/saauso-maybe.h"
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
#include "src/runtime/runtime-py-list.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/stable-merge-sort.h"
#include "src/utils/utils.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static
Tagged<PyListKlass> PyListKlass::GetInstance(Isolate* isolate) {
  Tagged<PyListKlass> instance = isolate->py_list_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyListKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_list_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyListKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 设置内建布局字段
  set_native_layout_kind(NativeLayoutKind::kList);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.init_instance_ = &Virtual_InitInstance;
  vtable_.len_ = &Virtual_Len;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.add_ = &Virtual_Add;
  vtable_.mul_ = &Virtual_Mul;
  vtable_.subscr_ = &Virtual_Subscr;
  vtable_.store_subscr_ = &Virtual_StoreSubscr;
  vtable_.del_subscr_ = &Virtual_DelSubscr;
  vtable_.less_ = &Virtual_Less;
  vtable_.iter_ = &Virtual_Iter;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyListKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate, PyListBuiltinMethods::Install(
                                   isolate, klass_properties, type_object()));

  // 设置类名
  set_name(PyString::New(isolate, "list"));

  return JustVoid();
}

void PyListKlass::Finalize(Isolate* isolate) {
  isolate->set_py_list_klass(Tagged<PyListKlass>::null());
}

MaybeHandle<PyObject> PyListKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> receiver_klass = receiver_type->own_klass();

  assert(receiver_klass->native_layout_kind() == NativeLayoutKind::kList);

  Handle<PyList> result = isolate->factory()->NewPyListLike(
      receiver_klass, PyList::kMinimumCapacity);

  return result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_InitInstance(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> instance_klass =
      PyObject::ResolveObjectKlass(instance, isolate);

  bool is_valid_klass = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_valid_klass,
      Runtime_IsSubtype(instance_klass, PyListKlass::GetInstance(isolate)));

  if (!is_valid_klass) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "descriptor '__init__' requires a 'list' object but received a '%s'",
        PyObject::GetTypeName(instance, isolate)->buffer());
    return kNullMaybeHandle;
  }
  assert(instance_klass->native_layout_kind() == NativeLayoutKind::kList);

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                       "list() takes no keyword arguments");
    return kNullMaybeHandle;
  }
  if (argc > 1) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "list expected at most 1 argument, got %" PRId64, argc);
    return kNullMaybeHandle;
  }
  if (argc == 1) {
    RETURN_ON_EXCEPTION(isolate, Runtime_ExtendListByItratableObject(
                                     isolate, Handle<PyList>::cast(instance),
                                     pos_args->Get(0)));
  }
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> PyListKlass::Virtual_Len(Isolate* isolate,
                                               Handle<PyObject> self) {
  return Handle<PyObject>(PySmi::FromInt(Handle<PyList>::cast(self)->length()));
}

MaybeHandle<PyObject> PyListKlass::Virtual_Repr(Isolate* isolate,
                                                Handle<PyObject> self) {
  Handle<PyString> repr;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, repr, Runtime_NewListRepr(isolate, Handle<PyList>::cast(self)));
  return repr;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Str(Isolate* isolate,
                                               Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

MaybeHandle<PyObject> PyListKlass::Virtual_Add(Isolate* isolate,
                                               Handle<PyObject> self,
                                               Handle<PyObject> other) {
  auto list1 = Handle<PyList>::cast(self);
  if (!IsPyList(other)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "can only concatenate list (not \"%s\") to list",
                        PyObject::GetTypeName(other, isolate)->buffer());
    return kNullMaybeHandle;
  }
  auto list2 = Handle<PyList>::cast(other);

  auto new_result = PyList::New(isolate, list1->length() + list2->length());
  for (auto i = 0; i < list1->length(); ++i) {
    PyList::Append(new_result, list1->Get(i), isolate);
  }

  for (auto i = 0; i < list2->length(); ++i) {
    PyList::Append(new_result, list2->Get(i), isolate);
  }

  return new_result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Mul(Isolate* isolate,
                                               Handle<PyObject> self,
                                               Handle<PyObject> coeff) {
  if (!IsPySmi(coeff)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "can't multiply sequence by non-int of type '%s'",
                        PyObject::GetTypeName(coeff, isolate)->buffer());
    return kNullMaybeHandle;
  }

  auto list = Handle<PyList>::cast(self);
  auto decoded_coeff = std::max(static_cast<int64_t>(0),
                                PySmi::ToInt(Handle<PySmi>::cast(coeff)));

  auto result = PyList::New(isolate, list->length() * decoded_coeff);
  while (decoded_coeff-- > 0) {
    for (int i = 0; i < list->length(); ++i) {
      PyList::Append(result, list->Get(i), isolate);
    }
  }

  return result;
}

MaybeHandle<PyObject> PyListKlass::Virtual_Subscr(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> subscr) {
  if (!IsPySmi(subscr)) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kTypeError,
                        "list indices must be integers or slices, not %s",
                        PyObject::GetTypeName(subscr, isolate)->buffer());
    return kNullMaybeHandle;
  }

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  return Handle<PyList>::cast(self)->Get(decoded_subscr);
}

MaybeHandle<PyObject> PyListKlass::Virtual_StoreSubscr(Isolate* isolate,
                                                       Handle<PyObject> self,
                                                       Handle<PyObject> subscr,
                                                       Handle<PyObject> value) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                       "list assignment index out of range");
    return kNullMaybeHandle;
  }

  list->Set(decoded_subscr, value);
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> PyListKlass::Virtual_DelSubscr(Isolate* isolate,
                                                     Handle<PyObject> self,
                                                     Handle<PyObject> subscr) {
  auto list = Handle<PyList>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            list->length())) {
    Runtime_ThrowError(isolate, ExceptionType::kIndexError,
                       "list assignment index out of range");
    return kNullMaybeHandle;
  }

  list->RemoveByIndex(decoded_subscr);
  return handle(isolate->py_none_object());
}

Maybe<bool> PyListKlass::Virtual_Less(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> other) {
  auto list_l = Handle<PyList>::cast(self);
  if (!IsPyList(other)) {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "'<' not supported between instances of 'list' and '%s'",
        PyObject::GetTypeName(other, isolate)->buffer());
    return kNullMaybe;
  }
  auto list_r = Handle<PyList>::cast(other);
  auto min_len = std::min(list_l->length(), list_r->length());

  for (auto i = 0; i < min_len; ++i) {
    auto l = list_l->Get(i);
    auto r = list_r->Get(i);

    Maybe<bool> less_mb = PyObject::LessBool(isolate, l, r);
    if (less_mb.IsNothing()) {
      return kNullMaybe;
    }
    if (less_mb.ToChecked()) {
      return Maybe<bool>(true);
    }

    Maybe<bool> greater_mb = PyObject::GreaterBool(isolate, l, r);
    if (greater_mb.IsNothing()) {
      return kNullMaybe;
    }
    if (greater_mb.ToChecked()) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(list_l->length() < list_r->length());
}

Maybe<bool> PyListKlass::Virtual_Contains(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> target) {
  auto list = Handle<PyList>::cast(self);
  for (auto i = 0; i < list->length(); ++i) {
    Maybe<bool> eq = PyObject::EqualBool(isolate, list->Get(i), target);
    if (eq.IsNothing()) {
      return kNullMaybe;
    }
    if (eq.ToChecked()) {
      return Maybe<bool>(true);
    }
  }

  return Maybe<bool>(false);
}

Maybe<bool> PyListKlass::Virtual_Equal(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> target) {
  auto list1 = Handle<PyList>::cast(self);
  if (!IsPyList(target)) {
    return Maybe<bool>(false);
  }
  auto list2 = Handle<PyList>::cast(target);

  if (list1->length() != list2->length()) {
    return Maybe<bool>(false);
  }

  for (auto i = 0; i < list1->length(); ++i) {
    Maybe<bool> eq = PyObject::EqualBool(isolate, list1->Get(i), list2->Get(i));
    if (eq.IsNothing()) {
      return kNullMaybe;
    }
    if (!eq.ToChecked()) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(true);
}

MaybeHandle<PyObject> PyListKlass::Virtual_Iter(Isolate* isolate,
                                                Handle<PyObject> object) {
  return isolate->factory()->NewPyListIterator(Handle<PyList>::cast(object));
}

size_t PyListKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyList));
}

void PyListKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyList(self));
  v->VisitPointer(&Tagged<PyList>::cast(self)->array_);
}

}  // namespace saauso::internal
