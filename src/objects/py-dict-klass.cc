// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-dict-klass.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "include/saauso-internal.h"
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
#include "src/utils/maybe.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static
Tagged<PyDictKlass> PyDictKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyDictKlass> instance = isolate->py_dict_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyDictKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_dict_klass(instance);
  }
  return instance;
}

void PyDictKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  set_native_layout_kind(NativeLayoutKind::kDict);
  set_native_layout_base(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.store_subscr = &Virtual_StoreSubscr;
  vtable_.del_subscr = &Virtual_DeleteSubscr;
  vtable_.contains = &Virtual_Contains;
  vtable_.iter = &Virtual_Iter;
  vtable_.new_instance = &Virtual_NewInstance;
  vtable_.init_instance = &Virtual_InitInstance;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyDictKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();

  // 安装内建方法
  RETURN_ON_EXCEPTION(isolate,
                      PyDictBuiltinMethods::Install(isolate, klass_properties));

  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 设置类名
  set_name(PyString::NewInstance("dict"));

  return JustVoid();
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Print(Handle<PyObject> self) {
  auto* isolate [[maybe_unused]] = Isolate::Current();
  auto dict = Handle<PyDict>::cast(self);
  std::printf("{");
  bool first = true;
  for (int64_t i = 0; i < dict->capacity(); ++i) {
    auto key = dict->data()->Get(i << 1);
    if (!key.is_null()) {
      if (!first) {
        std::printf(", ");
      }
      first = false;

      RETURN_ON_EXCEPTION(isolate, PyObject::Print(Handle<PyObject>(key)));

      std::printf(": ");

      auto value = handle(dict->data()->Get((i << 1) + 1));
      RETURN_ON_EXCEPTION(isolate, PyObject::Print(value));
    }
  }
  std::printf("}");
  return handle(isolate->py_none_object());
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Iter(Handle<PyObject> self) {
  return Isolate::Current()->factory()->NewPyDictKeyIterator(self);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Len(Handle<PyObject> self) {
  auto value = Handle<PyDict>::cast(self)->occupied();
  return Handle<PyObject>(PySmi::FromInt(value));
}

Maybe<bool> PyDictKlass::Virtual_Equal(Handle<PyObject> self,
                                       Handle<PyObject> other) {
  auto* isolate [[maybe_unused]] = Isolate::Current();

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
          PyObject::EqualBool(Handle<PyObject>(v1),
                              Handle<PyObject>(v2_tagged)));

      if (!eq) {
        return Maybe<bool>(false);
      }
    }
  }
  return Maybe<bool>(true);
}

Maybe<bool> PyDictKlass::Virtual_NotEqual(Handle<PyObject> self,
                                          Handle<PyObject> other) {
  bool eq;
  ASSIGN_RETURN_ON_EXCEPTION(Isolate::Current(), eq,
                             Virtual_Equal(self, other));
  return Maybe<bool>(!eq);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_Subscr(Handle<PyObject> self,
                                                  Handle<PyObject> subscr) {
  return Runtime_DictGetItem(Handle<PyDict>::cast(self), subscr);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_StoreSubscr(Handle<PyObject> self,
                                                       Handle<PyObject> subscr,
                                                       Handle<PyObject> value) {
  return Runtime_DictSetItem(Handle<PyDict>::cast(self), subscr, value);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_DeleteSubscr(
    Handle<PyObject> self,
    Handle<PyObject> subscr) {
  return Runtime_DictDelItem(Handle<PyDict>::cast(self), subscr);
}

Maybe<bool> PyDictKlass::Virtual_Contains(Handle<PyObject> self,
                                          Handle<PyObject> subscr) {
  return Handle<PyDict>::cast(self)->ContainsKey(subscr);
}

MaybeHandle<PyObject> PyDictKlass::Virtual_NewInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self->native_layout_kind() == NativeLayoutKind::kDict);
  auto* isolate = Isolate::Current();
  bool is_exact_dict = klass_self == PyDictKlass::GetInstance();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  if (is_exact_dict) {
    if (argc > 1) {
      Runtime_ThrowErrorf(ExceptionType::kTypeError,
                          "dict expected at most 1 argument, got %" PRId64,
                          argc);
      return kNullMaybeHandle;
    }
  }

  Handle<PyDict> result = isolate->factory()->NewDictLike(
      klass_self, PyDict::kMinimumCapacity, !is_exact_dict);
  return result;
}

Maybe<void> PyDictKlass::Virtual_InitInstance(Tagged<Klass> klass_self,
                                              Handle<PyObject> instance,
                                              Handle<PyObject> args,
                                              Handle<PyObject> kwargs) {
  assert(klass_self->native_layout_kind() == NativeLayoutKind::kDict);
  auto* isolate = Isolate::Current();
  bool is_exact_dict = klass_self == PyDictKlass::GetInstance();
  auto result = Handle<PyDict>::cast(instance);
  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();
  Handle<PyObject> init_method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, result, ST(init), init_method));

  if (init_method.is_null()) {
    if (!is_exact_dict) {
      if (argc > 1) {
        Runtime_ThrowErrorf(ExceptionType::kTypeError,
                            "dict expected at most 1 argument, got %" PRId64,
                            argc);
        return kNullMaybe;
      }
    }

    bool ok = false;
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate, ok, Runtime_InitDictFromArgsKwargs(result, args, kwargs));
    if (!ok) {
      return kNullMaybe;
    }
    return JustVoid();
  }

  if (Execution::Call(isolate, init_method, result, pos_args,
                      Handle<PyDict>::cast(kwargs))
          .IsEmpty()) {
    return kNullMaybe;
  }
  return JustVoid();
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
void PyDictKlass::Finalize() {
  Isolate::Current()->set_py_dict_klass(Tagged<PyDictKlass>::null());
}

}  // namespace saauso::internal
