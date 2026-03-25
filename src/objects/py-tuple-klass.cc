// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-tuple-klass.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "include/saauso-maybe.h"
#include "src/builtins/builtins-py-tuple-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
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
#include "src/runtime/runtime-py-tuple.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

Tagged<PyTupleKlass> PyTupleKlass::GetInstance(Isolate* isolate) {
  Tagged<PyTupleKlass> instance = isolate->py_tuple_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyTupleKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_tuple_klass(instance);
  }
  return instance;
}

void PyTupleKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  set_native_layout_kind(NativeLayoutKind::kTuple);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

  // 初始化虚函数表
  vtable_.Clear();
  // 注意，tuple是不可变类型，在Python中只有__new__，没有__init__。
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.len_ = &Virtual_Len;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.subscr_ = &Virtual_Subscr;
  vtable_.store_subscr_ = &Virtual_StoreSubscr;
  vtable_.del_subscr_ = &Virtual_DelSubscr;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.iter_ = &Virtual_Iter;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyTupleKlass::Initialize(Isolate* isolate) {
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

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate, PyTupleBuiltinMethods::Install(
                                   isolate, klass_properties, type_object()));

  set_name(PyString::NewInstance("tuple"));

  return JustVoid();
}

void PyTupleKlass::Finalize(Isolate* isolate) {
  isolate->set_py_tuple_klass(Tagged<PyTupleKlass>::null());
}

// 构造实例对象时，针对用户输入参数的校验，不需要特判当前要创建的是tuple-like还是tuple-exact对象！
// 直接严格按照创建标准tuple-exact对象的校验要求即可，也不需要考虑tuple-like类的__init__方法！
//
// 可参考以下例子：
// ```
// >>> class T(tuple):
// ...   def __init__(self, x, y, z):
// ...     self.x = x
// ...     self.y = y
// ...     self.z = z
// ...
// >>> t = T()
// Traceback (most recent call last):
//   File "<stdin>", line 1, in <module>
// TypeError: T.__init__() missing 3 required positional arguments: 'x', 'y',
// and 'z'
// >>> t = T(1, 2, 3)
// Traceback (most recent call last):
//   File "<stdin>", line 1, in <module>
// TypeError: tuple expected at most 1 argument, got 3
// >>> t = T([1, 2, 3])
// Traceback (most recent call last):
//   File "<stdin>", line 1, in <module>
// TypeError: T.__init__() missing 2 required positional arguments: 'y' and 'z'
// ```
MaybeHandle<PyObject> PyTupleKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> receiver_klass = receiver_type->own_klass();

  assert(receiver_klass->native_layout_kind() == NativeLayoutKind::kTuple);

  bool is_exact_tuple = receiver_klass == PyTupleKlass::GetInstance(isolate);

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "tuple() takes no keyword arguments\n");
    return kNullMaybeHandle;
  }

  if (argc > 1) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "tuple expected at most 1 argument, got %" PRId64 "\n",
                        argc);
    return kNullMaybeHandle;
  }

  Handle<PyTuple> result;
  if (argc == 1) {
    Handle<PyObject> iterable = pos_args->Get(0);
    // 对齐 CPython 3.12：
    // 如果要创建一个tuple-exact实例，同时传入的参数恰好是一个tuple-exact对象，
    // 那么不分配内存，直接返回即可。
    if (is_exact_tuple && IsPyTupleExact(iterable)) {
      return iterable;
    }

    Handle<PyTuple> unpacked;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, unpacked,
                               Runtime_UnpackIterableObjectToTuple(iterable));

    int64_t unpacked_length = unpacked->length();
    result =
        isolate->factory()->NewPyTupleLike(receiver_klass, unpacked_length);

    for (int64_t i = 0; i < unpacked_length; ++i) {
      result->SetInternal(i, unpacked->GetTagged(i));
    }
  } else {
    result = isolate->factory()->NewPyTupleLike(receiver_klass, 0);
  }

  return result;
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyTuple>::cast(self)->length()));
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Repr(Isolate* isolate,
                                                 Handle<PyObject> self) {
  Handle<PyString> repr;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, repr,
      Runtime_NewTupleRepr(isolate, Handle<PyTuple>::cast(self)));
  return repr;
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Str(Isolate* isolate,
                                                Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
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

Maybe<bool> PyTupleKlass::Virtual_Contains(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> target) {
  auto tuple = Handle<PyTuple>::cast(self);
  for (auto i = 0; i < tuple->length(); ++i) {
    bool eq;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, eq,
                               PyObject::EqualBool(isolate, tuple->Get(i),
                                                   target));
    if (eq) {
      return Maybe<bool>(true);
    }
  }
  return Maybe<bool>(false);
}

Maybe<bool> PyTupleKlass::Virtual_Equal(Isolate* isolate,
                                        Handle<PyObject> self,
                                        Handle<PyObject> other) {
  if (!IsPyTuple(other)) {
    return Maybe<bool>(false);
  }

  auto tuple1 = Handle<PyTuple>::cast(self);
  auto tuple2 = Handle<PyTuple>::cast(other);

  if (tuple1->length() != tuple2->length()) {
    return Maybe<bool>(false);
  }

  for (auto i = 0; i < tuple1->length(); ++i) {
    bool eq;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, eq,
        PyObject::EqualBool(isolate, tuple1->Get(i), tuple2->Get(i)));
    if (!eq) {
      return Maybe<bool>(false);
    }
  }

  return Maybe<bool>(true);
}

MaybeHandle<PyObject> PyTupleKlass::Virtual_Iter(Handle<PyObject> self) {
  return Isolate::Current()->factory()->NewPyTupleIterator(
      Handle<PyTuple>::cast(self));
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
