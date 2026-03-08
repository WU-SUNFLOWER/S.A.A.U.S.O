// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string-klass.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "src/builtins/builtins-py-string-methods.h"
#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/handles/tagged.h"
#include "src/heap/factory.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-py-string.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/string-table.h"
#include "src/utils/maybe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

////////////////////////////////////////////////////////////////////

// static

// static
Tagged<PyStringKlass> PyStringKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyStringKlass> instance = isolate->py_string_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyStringKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_string_klass(instance);
  }
  return instance;
}

////////////////////////////////////////////////////////////////////

void PyStringKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  set_native_layout_kind(NativeLayoutKind::kString);
  set_native_layout_base(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.new_instance = &Virtual_NewInstance;
  vtable_.init_instance = &Virtual_InitInstance;
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.less = &Virtual_Less;
  vtable_.greater = &Virtual_Greater;
  vtable_.le = &Virtual_LessEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.contains = &Virtual_Contains;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.add = &Virtual_Add;
  vtable_.print = &Virtual_Print;
  vtable_.hash = &Virtual_Hash;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyStringKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类属性表
  auto klass_properties = PyDict::NewInstance();

  // 安装内建方法
  RETURN_ON_EXCEPTION(
      isolate, PyStringBuiltinMethods::Install(isolate, klass_properties));

  // 初始化类字典
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 设置类名
  set_name(PyString::NewInstance("str"));

  return JustVoid();
}

void PyStringKlass::Finalize() {
  Isolate::Current()->set_py_string_klass(Tagged<PyStringKlass>::null());
}

MaybeHandle<PyObject> PyStringKlass::Virtual_NewInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self->native_layout_kind() == NativeLayoutKind::kString);
  auto* isolate = Isolate::Current();
  bool is_exact_str = klass_self == PyStringKlass::GetInstance();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();

  if (is_exact_str) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "str() takes no keyword arguments\n");
      return kNullMaybeHandle;
    }
  }

  if (is_exact_str && argc == 1) {
    Handle<PyObject> value = pos_args->Get(0);
    if (IsPyStringExact(value)) {
      return value;
    }
  }

  auto probe =
      isolate->factory()->NewRawStringLike(klass_self, 0, false, !is_exact_str);
  if (!is_exact_str) {
    auto properties = PyObject::GetProperties(probe);
    RETURN_ON_EXCEPTION(
        isolate, PyDict::Put(properties, ST(class), klass_self->type_object()));
  }

  Handle<PyObject> init_method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, probe, ST(init), init_method));

  if (init_method.is_null()) {
    if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "str() takes no keyword arguments\n");
      return kNullMaybeHandle;
    }

    if (argc == 0) {
      return probe;
    }

    if (argc == 1) {
      Handle<PyObject> value = pos_args->Get(0);
      Handle<PyString> converted;
      if (!Runtime_NewStr(value).ToHandle(&converted)) {
        return kNullMaybeHandle;
      }
      if (is_exact_str) {
        return converted;
      }
      auto result = isolate->factory()->NewRawStringLike(
          klass_self, converted->length(), false, true);
      auto properties = PyObject::GetProperties(result);
      RETURN_ON_EXCEPTION(isolate, PyDict::Put(properties, ST(class),
                                               klass_self->type_object()));
      std::memcpy(result->writable_buffer(), converted->buffer(),
                  static_cast<size_t>(converted->length()));
      return result;
    }

    if (argc == 2 || argc == 3) {
      Handle<PyObject> value = pos_args->Get(0);
      if (IsPyString(value)) {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "decoding str is not supported\n");
        return kNullMaybeHandle;
      }
      auto type_name = PyObject::GetKlass(value)->name();
      Runtime_ThrowErrorf(
          ExceptionType::kTypeError,
          "decoding to str: need a bytes-like object, %s found\n",
          type_name->buffer());
      return kNullMaybeHandle;
    }

    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "str() takes at most 3 arguments (%" PRId64 " given)\n",
                        argc);
    return kNullMaybeHandle;
  }

  return probe;
}

Maybe<void> PyStringKlass::Virtual_InitInstance(Tagged<Klass> klass_self,
                                                Handle<PyObject> instance,
                                                Handle<PyObject> args,
                                                Handle<PyObject> kwargs) {
  assert(klass_self->native_layout_kind() == NativeLayoutKind::kString);
  auto* isolate = Isolate::Current();
  auto probe = Handle<PyString>::cast(instance);
  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  Handle<PyObject> init_method;
  RETURN_ON_EXCEPTION(isolate, Runtime_FindPropertyInInstanceTypeMro(
                                   isolate, probe, ST(init), init_method));
  if (init_method.is_null()) {
    return JustVoid();
  }
  if (Execution::Call(isolate, init_method, probe, pos_args,
                      Handle<PyDict>::cast(kwargs))
          .IsEmpty()) {
    return kNullMaybe;
  }
  return JustVoid();
}

MaybeHandle<PyObject> PyStringKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyString>::cast(self)->length()));
}

Maybe<bool> PyStringKlass::Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return Maybe<bool>(false);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(s1->IsEqualTo(*s2));
}

Maybe<bool> PyStringKlass::Virtual_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return Maybe<bool>(true);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(!s1->IsEqualTo(*s2));
}

Maybe<bool> PyStringKlass::Virtual_Less(Handle<PyObject> self,
                                        Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "'<' not supported between instances of 'str' and '%s'\n",
        other_name->buffer());
    return kNullMaybe;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(s1->IsLessThan(*s2));
}

Maybe<bool> PyStringKlass::Virtual_Greater(Handle<PyObject> self,
                                           Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "'>' not supported between instances of 'str' and '%s'\n",
        other_name->buffer());
    return kNullMaybe;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(s1->IsGreaterThan(*s2));
}

Maybe<bool> PyStringKlass::Virtual_LessEqual(Handle<PyObject> self,
                                             Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "'<=' not supported between instances of 'str' and '%s'\n",
        other_name->buffer());
    return kNullMaybe;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(s1->IsEqualTo(*s2) || s1->IsLessThan(*s2));
}

Maybe<bool> PyStringKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                                Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    Runtime_ThrowErrorf(
        ExceptionType::kTypeError,
        "'>=' not supported between instances of 'str' and '%s'\n",
        other_name->buffer());
    return kNullMaybe;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Maybe<bool>(s1->IsEqualTo(*s2) || s1->IsGreaterThan(*s2));
}

Maybe<bool> PyStringKlass::Virtual_Contains(Handle<PyObject> self,
                                            Handle<PyObject> target) {
  if (!IsPyString(target)) {
    return Maybe<bool>(false);
  }

  auto s = Handle<PyString>::cast(self);
  auto pattern = Handle<PyString>::cast(target);
  return Maybe<bool>(s->Contains(pattern));
}

MaybeHandle<PyObject> PyStringKlass::Virtual_Subscr(Handle<PyObject> self,
                                                    Handle<PyObject> subscr) {
  auto s = Handle<PyString>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            s->length())) [[unlikely]] {
    Runtime_ThrowError(ExceptionType::kIndexError, "string index out of range");
    return kNullMaybeHandle;
  }

  return PyString::NewInstance(s->buffer() + decoded_subscr,
                               static_cast<int64_t>(1));
}

MaybeHandle<PyObject> PyStringKlass::Virtual_Add(Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  if (!IsPyString(other)) [[unlikely]] {
    auto other_klass = PyObject::GetKlass(other);
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "can only concatenate str (not \"%s\") to str\n",
                        other_klass->name()->buffer());
    return kNullMaybeHandle;
  }

  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return PyString::Append(s1, s2);
}

MaybeHandle<PyObject> PyStringKlass::Virtual_Print(Handle<PyObject> self) {
  auto s = Handle<PyString>::cast(self);
  std::printf("%s", s->buffer());
  return handle(Isolate::Current()->py_none_object());
}

Maybe<uint64_t> PyStringKlass::Virtual_Hash(Handle<PyObject> self) {
  return Maybe<uint64_t>(Handle<PyString>::cast(self)->GetHash());
}

size_t PyStringKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(IsPyString(self));
  return PyString::ComputeObjectSize(Tagged<PyString>::cast(self)->length());
}

void PyStringKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  // 字符串数据直接内联在PyString结构体中，不需要处理额外的内部拷贝
}

}  // namespace saauso::internal
