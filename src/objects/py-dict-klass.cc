// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-klass.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "include/saauso-internal.h"
#include "include/saauso-maybe.h"
#include "src/builtins/builtins-py-dict-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
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
#include "src/runtime/runtime-py-dict.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static
Tagged<PyDictKlass> PyDictKlass::GetInstance(Isolate* isolate) {
  Tagged<PyDictKlass> instance = isolate->py_dict_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_klass(instance);
  }
  return instance;
}

void PyDictKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  set_native_layout_kind(NativeLayoutKind::kDict);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.len_ = &Virtual_Len;
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.subscr_ = &Virtual_Subscr;
  vtable_.store_subscr_ = &Virtual_StoreSubscr;
  vtable_.del_subscr_ = &Virtual_DeleteSubscr;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.iter_ = &Virtual_Iter;
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.init_instance_ = &Virtual_InitInstance;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyDictKlass::Initialize(Isolate* isolate) {
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
  RETURN_ON_EXCEPTION(isolate, PyDictBuiltinMethods::Install(
                                   isolate, klass_properties, type_object()));

  // 设置类名
  set_name(PyString::NewInstance("dict"));

  return JustVoid();
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Iter(Isolate* isolate,
                                                Handle<PyObject> self) {
  return isolate->factory()->NewPyDictKeyIterator(self);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Len(Isolate* isolate,
                                               Handle<PyObject> self) {
  auto value = Handle<PyDict>::cast(self)->occupied();
  return Handle<PyObject>(PySmi::FromInt(value));
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Repr(Isolate* isolate,
                                                Handle<PyObject> self) {
  Handle<PyString> repr;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, repr, Runtime_NewDictRepr(isolate, Handle<PyDict>::cast(self)));
  return repr;
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Str(Isolate* isolate,
                                               Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

Maybe<bool> PyDictKlass::Virtual_Equal(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> other) {
  if (self.is_identical_to(other)) {
    return Maybe<bool>(true);
  }
  if (!IsPyDict(other)) {
    return Maybe<bool>(false);
  }

  auto d1 = Handle<PyDict>::cast(self);
  auto d2 = Handle<PyDict>::cast(other);

  if (d1->occupied() != d2->occupied()) {
    return Maybe<bool>(false);
  }

  for (int64_t i = 0; i < d1->capacity(); ++i) {
    auto k1 = d1->data()->Get(i << 1);
    if (!k1.is_null()) {
      auto v1 = d1->data()->Get((i << 1) + 1);
      Tagged<PyObject> v2_tagged;
      bool found = false;
      ASSIGN_RETURN_ON_EXCEPTION(isolate, found, d2->GetTagged(k1, v2_tagged));
      if (!found) {
        return Maybe<bool>(false);
      }

      bool eq;
      ASSIGN_RETURN_ON_EXCEPTION(
          isolate, eq,
          PyObject::EqualBool(isolate, Handle<PyObject>(v1),
                              Handle<PyObject>(v2_tagged)));

      if (!eq) {
        return Maybe<bool>(false);
      }
    }
  }
  return Maybe<bool>(true);
}

Maybe<bool> PyDictKlass::Virtual_NotEqual(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other) {
  bool eq;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, eq, Virtual_Equal(isolate, self, other));
  return Maybe<bool>(!eq);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Subscr(Isolate* isolate,
                                                  Handle<PyObject> self,
                                                  Handle<PyObject> subscr) {
  return Runtime_DictGetItem(isolate, Handle<PyDict>::cast(self), subscr);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_StoreSubscr(Isolate* isolate,
                                                       Handle<PyObject> self,
                                                       Handle<PyObject> subscr,
                                                       Handle<PyObject> value) {
  return Runtime_DictSetItem(isolate, Handle<PyDict>::cast(self), subscr,
                             value);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_DeleteSubscr(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> subscr) {
  return Runtime_DictDelItem(isolate, Handle<PyDict>::cast(self), subscr);
}

Maybe<bool> PyDictKlass::Virtual_Contains(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  return Handle<PyDict>::cast(self)->ContainsKey(subscr);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> receiver_klass = receiver_type->own_klass();

  assert(receiver_klass->native_layout_kind() == NativeLayoutKind::kDict);

  Handle<PyDict> result =
      isolate->factory()->NewDictLike(receiver_klass, PyDict::kMinimumCapacity);
  return result;
}

MaybeHandle<PyObject> PyDictKlass::Virtual_InitInstance(
    Isolate* isolate,
    Handle<PyObject> instance,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> instance_klass = PyObject::GetKlass(instance);

  bool is_valid_klass = false;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, is_valid_klass,
      Runtime_IsSubtype(instance_klass, PyDictKlass::GetInstance(isolate)));

  if (!is_valid_klass) [[unlikely]] {
    Runtime_ThrowErrorf(
        isolate, ExceptionType::kTypeError,
        "descriptor '__init__' requires a 'dict' object but received a '%s'",
        instance_klass->name()->buffer());
    return kNullMaybeHandle;
  }
  assert(instance_klass->native_layout_kind() == NativeLayoutKind::kDict);

  RETURN_ON_EXCEPTION(
      isolate, Runtime_InitDictFromArgsKwargs(
                   isolate, Handle<PyDict>::cast(instance), args, kwargs));

  return handle(isolate->py_none_object());
}

// static
size_t PyDictKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyDict));
}

// static
void PyDictKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  assert(IsPyDict(self));
  auto dict = Tagged<PyDict>::cast(self);
  v->VisitPointer(&dict->data_);
}

// static
void PyDictKlass::Finalize(Isolate* isolate) {
  isolate->set_py_dict_klass(Tagged<PyDictKlass>::null());
}

}  // namespace saauso::internal
