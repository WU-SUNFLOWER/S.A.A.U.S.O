// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string-klass.h"

#include <cinttypes>

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

void PyStringKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  set_native_layout_kind(NativeLayoutKind::kString);
  set_native_layout_base(PyObjectKlass::GetInstance());

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.len_ = &Virtual_Len;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.less_ = &Virtual_Less;
  vtable_.greater_ = &Virtual_Greater;
  vtable_.le_ = &Virtual_LessEqual;
  vtable_.ge_ = &Virtual_GreaterEqual;
  vtable_.contains_ = &Virtual_Contains;
  vtable_.subscr_ = &Virtual_Subscr;
  vtable_.add_ = &Virtual_Add;
  vtable_.print_ = &Virtual_Print;
  vtable_.hash_ = &Virtual_Hash;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyStringKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  auto klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 设置类名
  set_name(PyString::NewInstance("str"));

  // 安装内建方法
  RETURN_ON_EXCEPTION(
      isolate, PyStringBuiltinMethods::Install(isolate, klass_properties));

  return JustVoid();
}

void PyStringKlass::Finalize(Isolate* isolate) {
  isolate->set_py_string_klass(Tagged<PyStringKlass>::null());
}

MaybeHandle<PyObject> PyStringKlass::Virtual_NewInstance(
    Isolate* isolate,
    Handle<PyTypeObject> receiver_type,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  Tagged<Klass> receiver_klass = receiver_type->own_klass();

  assert(receiver_klass->native_layout_kind() == NativeLayoutKind::kString);

  bool is_exact_str = receiver_klass == PyStringKlass::GetInstance();

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    Runtime_ThrowError(ExceptionType::kTypeError,
                       "str() takes no keyword arguments\n");
    return kNullMaybeHandle;
  }

  Handle<PyObject> input_value;
  Handle<PyString> result = isolate->factory()->NewRawStringLike(
      receiver_klass, 0, false, !is_exact_str);

  if (argc == 0) {
    goto default_return_result;
  }

  input_value = pos_args->Get(0);
  if (argc == 1) {
    if (is_exact_str && IsPyStringExact(input_value)) {
      return input_value;
    }

    Handle<PyString> converted;
    ASSIGN_RETURN_ON_EXCEPTION(isolate, converted, Runtime_NewStr(input_value));
    if (is_exact_str) {
      return converted;
    }

    result = isolate->factory()->NewStringLike(
        receiver_klass, converted->buffer(), converted->length(), true);
    goto default_return_result;
  }

  if (argc == 2 || argc == 3) {
    if (IsPyString(input_value)) {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "decoding str is not supported\n");
    } else {
      auto type_name = PyObject::GetKlass(input_value)->name();
      Runtime_ThrowErrorf(
          ExceptionType::kTypeError,
          "decoding to str: need a bytes-like object, %s found\n",
          type_name->buffer());
    }
    return kNullMaybeHandle;
  }

  if (argc > 3) {
    Runtime_ThrowErrorf(ExceptionType::kTypeError,
                        "str() takes at most 3 arguments (%" PRId64 " given)\n",
                        argc);
    return kNullMaybeHandle;
  }

default_return_result:
  if (!is_exact_str) {
    auto properties = PyObject::GetProperties(result);
    RETURN_ON_EXCEPTION(isolate,
                        PyDict::Put(properties, ST(class), receiver_type));
  }
  return result;
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
