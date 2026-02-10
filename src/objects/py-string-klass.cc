// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string-klass.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
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
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"


namespace saauso::internal {

namespace {

Handle<PyObject> NativeMethod_Upper(Handle<PyObject> self,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  EscapableHandleScope scope;

  auto str_object = Handle<PyString>::cast(self);
  auto result =
      PyString::NewInstance(str_object->buffer(), str_object->length());

  for (auto i = 0; i < result->length(); ++i) {
    char ch = result->Get(i);
    if (InRangeWithRightClose(ch, 'a', 'z')) {
      result->Set(i, ch - 'a' + 'A');
    }
  }

  return scope.Escape(result);
}

Handle<PyObject> NativeMethod_Index(Handle<PyObject> self,
                                    Handle<PyTuple> args,
                                    Handle<PyDict> kwargs) {
  EscapableHandleScope scope;
  auto str_object = Handle<PyString>::cast(self);

  if (!kwargs.is_null() && kwargs->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str.index() takes no keyword arguments\n");
    std::exit(1);
  }

  int64_t argc = args.is_null() ? 0 : args->length();
  if (argc < 1) {
    std::fprintf(
        stderr,
        "TypeError: str.index() takes at least 1 argument (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }
  if (argc > 3) {
    std::fprintf(
        stderr,
        "TypeError: str.index() takes at most 3 arguments (%lld given)\n",
        static_cast<long long>(argc));
    std::exit(1);
  }

  auto target = args->Get(0);
  if (!IsPyString(target)) {
    auto type_name = PyObject::GetKlass(target)->name();
    std::fprintf(stderr, "TypeError: must be str, not %s\n",
                 type_name->buffer());
    std::exit(1);
  }

  auto target_str = Handle<PyString>::cast(target);

  int64_t length = str_object->length();
  int64_t begin = 0;
  int64_t end = length;
  if (argc >= 2) {
    begin = Runtime_DecodeIntLikeOrDie(args->GetTagged(1));
  }
  if (argc >= 3) {
    end = Runtime_DecodeIntLikeOrDie(args->GetTagged(2));
  }

  if (begin < 0) {
    begin += length;
  }
  if (end < 0) {
    end += length;
  }

  begin = std::min(std::max(static_cast<int64_t>(0), begin), length);
  end = std::min(std::max(static_cast<int64_t>(0), end), length);

  int64_t result = PyString::kNotFound;
  if (begin <= end) {
    result = str_object->IndexOf(target_str, begin, end);
  }
  if (result == PyString::kNotFound) {
    std::fprintf(stderr, "ValueError: substring not found\n");
    std::exit(1);
  }

  return scope.Escape(handle(PySmi::FromInt(result)));
}

}  // namespace

////////////////////////////////////////////////////////////////////

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

  // 初始化虚函数表
  vtable_.construct_instance = &Virtual_ConstructInstance;
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

void PyStringKlass::Initialize() {
  // 初始化类属性表
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("upper");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Upper, prop_name));

  prop_name = PyString::NewInstance("index");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Index, prop_name));

  // 初始化类字典
  set_klass_properties(klass_properties);

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("str"));
}

void PyStringKlass::Finalize() {
  Isolate::Current()->set_py_string_klass(Tagged<PyStringKlass>::null());
}

Handle<PyObject> PyStringKlass::Virtual_ConstructInstance(
    Tagged<Klass> klass_self,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(klass_self == PyStringKlass::GetInstance());

  if (!kwargs.is_null() && Handle<PyDict>::cast(kwargs)->occupied() != 0) {
    std::fprintf(stderr, "TypeError: str() takes no keyword arguments\n");
    std::exit(1);
  }

  Handle<PyTuple> pos_args = Handle<PyTuple>::cast(args);
  int64_t argc = pos_args.is_null() ? 0 : pos_args->length();

  if (argc == 0) {
    return PyString::NewInstance("");
  }

  if (argc == 1) {
    Handle<PyObject> value = pos_args->Get(0);
    if (IsPyString(value)) {
      return value;
    }
    if (IsPySmi(value)) {
      return PyString::FromPySmi(Tagged<PySmi>::cast(*value));
    }
    if (IsPyFloat(value)) {
      return PyString::FromPyFloat(Handle<PyFloat>::cast(value));
    }
    if (IsPyBoolean(value)) {
      bool v = Tagged<PyBoolean>::cast(*value)->value();
      return PyString::NewInstance(v ? "True" : "False");
    }
    if (IsPyNone(value)) {
      return PyString::NewInstance("None");
    }

    Handle<PyObject> method =
        Runtime_FindPropertyInInstanceTypeMro(value, ST(str));
    if (!method.is_null()) {
      Handle<PyObject> result = Isolate::Current()->interpreter()->CallPython(
          method, value, Handle<PyTuple>::null(), Handle<PyDict>::null());
      if (!IsPyString(result)) {
        std::fprintf(stderr, "TypeError: __str__ returned non-string\n");
        std::exit(1);
      }
      return result;
    }

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "<object at %p>",
                  reinterpret_cast<void*>((*value).ptr()));
    return PyString::NewInstance(buffer);
  }

  if (argc == 2 || argc == 3) {
    Handle<PyObject> value = pos_args->Get(0);
    if (IsPyString(value)) {
      std::fprintf(stderr, "TypeError: decoding str is not supported\n");
      std::exit(1);
    }
    auto type_name = PyObject::GetKlass(value)->name();
    std::fprintf(
        stderr,
        "TypeError: decoding to str: need a bytes-like object, %.*s found\n",
        static_cast<int>(type_name->length()), type_name->buffer());
    std::exit(1);
  }

  std::fprintf(stderr,
               "TypeError: str() takes at most 3 arguments (%lld given)\n",
               static_cast<long long>(argc));
  std::exit(1);
}

Handle<PyObject> PyStringKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyString>::cast(self)->length()));
}

bool PyStringKlass::Virtual_Equal(Handle<PyObject> self,
                                  Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return false;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return s1->IsEqualTo(*s2);
}

bool PyStringKlass::Virtual_NotEqual(Handle<PyObject> self,
                                     Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return true;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return !s1->IsEqualTo(*s2);
}

bool PyStringKlass::Virtual_Less(Handle<PyObject> self,
                                 Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    std::fprintf(stderr,
                 "TypeError: '<' not supported between instances of 'str' and "
                 "'%.*s'\n",
                 static_cast<int>(other_name->length()), other_name->buffer());
    std::exit(1);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return s1->IsLessThan(*s2);
}

bool PyStringKlass::Virtual_Greater(Handle<PyObject> self,
                                    Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    std::fprintf(stderr,
                 "TypeError: '>' not supported between instances of 'str' and "
                 "'%.*s'\n",
                 static_cast<int>(other_name->length()), other_name->buffer());
    std::exit(1);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return s1->IsGreaterThan(*s2);
}

bool PyStringKlass::Virtual_LessEqual(Handle<PyObject> self,
                                      Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    std::fprintf(stderr,
                 "TypeError: '<=' not supported between instances of 'str' and "
                 "'%.*s'\n",
                 static_cast<int>(other_name->length()), other_name->buffer());
    std::exit(1);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return s1->IsEqualTo(*s2) || s1->IsLessThan(*s2);
}

bool PyStringKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                         Handle<PyObject> other) {
  if (!IsPyString(other)) {
    auto other_name = PyObject::GetKlass(other)->name();
    std::fprintf(stderr,
                 "TypeError: '>=' not supported between instances of 'str' and "
                 "'%.*s'\n",
                 static_cast<int>(other_name->length()), other_name->buffer());
    std::exit(1);
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return s1->IsEqualTo(*s2) || s1->IsGreaterThan(*s2);
}

bool PyStringKlass::Virtual_Contains(Handle<PyObject> self,
                                     Handle<PyObject> target) {
  if (!IsPyString(target)) {
    return false;
  }

  auto s = Handle<PyString>::cast(self);
  auto pattern = Handle<PyString>::cast(target);
  return s->Contains(pattern);
}

Handle<PyObject> PyStringKlass::Virtual_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  auto s = Handle<PyString>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            s->length())) [[unlikely]] {
    std::fprintf(stderr, "IndexError: string index out of range");
    std::exit(1);
  }

  return PyString::NewInstance(s->buffer() + decoded_subscr,
                               static_cast<int64_t>(1));
}

Handle<PyObject> PyStringKlass::Virtual_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  if (!IsPyString(other)) [[unlikely]] {
    auto other_klass = PyObject::GetKlass(other);
    std::fprintf(stderr,
                 "TypeError: can only concatenate str (not \"%.*s\") to str\n",
                 static_cast<int>(other_klass->name()->length()),
                 other_klass->name()->buffer());
    std::exit(1);
  }

  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return PyString::Append(s1, s2);
}

void PyStringKlass::Virtual_Print(Handle<PyObject> self) {
  auto s = Handle<PyString>::cast(self);
  std::printf("%.*s", static_cast<int>(s->length()), s->buffer());
}

uint64_t PyStringKlass::Virtual_Hash(Handle<PyObject> self) {
  return Handle<PyString>::cast(self)->GetHash();
}

size_t PyStringKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  assert(IsPyString(self));
  return PyString::ComputeObjectSize(Tagged<PyString>::cast(self)->length());
}

void PyStringKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  // 字符串数据直接内联在PyString结构体中，不需要处理额外的内部拷贝
}

}  // namespace saauso::internal
