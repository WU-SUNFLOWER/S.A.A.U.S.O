// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-string-klass.h"

#include <cstdio>
#include <cstdlib>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"
#include "src/utils/utils.h"

namespace saauso::internal {

namespace {

Handle<PyObject> NativeMethod_Upper(Handle<PyList> args,
                                    Handle<PyDict> kwargs) {
  auto str_object = Handle<PyString>::cast(args->Get(0));
  auto result =
      PyString::NewInstance(str_object->buffer(), str_object->length());

  for (auto i = 0; i < result->length(); ++i) {
    char ch = result->Get(i);
    if (InRangeWithRightClose(ch, 'a', 'z')) {
      result->Set(i, ch - 'a' + 'A');
    }
  }

  return result;
}

}  // namespace

////////////////////////////////////////////////////////////////////

Tagged<PyStringKlass> PyStringKlass::instance_(nullptr);

// static
Tagged<PyStringKlass> PyStringKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyStringKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

////////////////////////////////////////////////////////////////////

void PyStringKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.len = &Virtual_Len;
  vtable_.equal = &Virtual_Equal;
  vtable_.not_equal = &Virtual_NotEqual;
  vtable_.less = &Virtual_Less;
  vtable_.greater = &Virtual_Greater;
  vtable_.le = &Virtual_LessEqual;
  vtable_.ge = &Virtual_GreaterEqual;
  vtable_.subscr = &Virtual_Subscr;
  vtable_.add = &Virtual_Add;
  vtable_.print = &Virtual_Print;
  vtable_.hash = &Virtual_Hash;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;

  // 初始化类属性表
  auto klass_properties = PyDict::NewInstance();

  auto prop_name = PyString::NewInstance("upper");
  PyDict::Put(klass_properties, prop_name,
              PyFunction::NewInstance(&NativeMethod_Upper, prop_name));

  set_klass_properties(klass_properties);

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置类名
  set_name(PyString::NewInstance("str"));
}

void PyStringKlass::Finalize() {
  instance_ = Tagged<PyStringKlass>::Null();
}

Handle<PyObject> PyStringKlass::Virtual_Len(Handle<PyObject> self) {
  return Handle<PyObject>(
      PySmi::FromInt(Handle<PyString>::cast(self)->length()));
}

Tagged<PyBoolean> PyStringKlass::Virtual_Equal(Handle<PyObject> self,
                                               Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return Universe::py_false_object_;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2));
}

Tagged<PyBoolean> PyStringKlass::Virtual_NotEqual(Handle<PyObject> self,
                                                  Handle<PyObject> other) {
  if (!IsPyString(other)) {
    return Universe::py_true_object_;
  }
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(!s1->IsEqualTo(*s2));
}

Tagged<PyBoolean> PyStringKlass::Virtual_Less(Handle<PyObject> self,
                                              Handle<PyObject> other) {
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(s1->IsLessThan(*s2));
}

Tagged<PyBoolean> PyStringKlass::Virtual_Greater(Handle<PyObject> self,
                                                 Handle<PyObject> other) {
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(s1->IsGreaterThan(*s2));
}

Tagged<PyBoolean> PyStringKlass::Virtual_LessEqual(Handle<PyObject> self,
                                                   Handle<PyObject> other) {
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2) || s1->IsLessThan(*s2));
}

Tagged<PyBoolean> PyStringKlass::Virtual_GreaterEqual(Handle<PyObject> self,
                                                      Handle<PyObject> other) {
  auto s1 = Handle<PyString>::cast(self);
  auto s2 = Handle<PyString>::cast(other);
  return Universe::ToPyBoolean(s1->IsEqualTo(*s2) || s1->IsGreaterThan(*s2));
}

Handle<PyObject> PyStringKlass::Virtual_Subscr(Handle<PyObject> self,
                                               Handle<PyObject> subscr) {
  auto s = Handle<PyString>::cast(self);

  auto decoded_subscr = PySmi::ToInt(Handle<PySmi>::cast(subscr));
  if (!InRangeWithRightOpen(decoded_subscr, static_cast<int64_t>(0),
                            s->length())) [[unlikely]] {
    std::printf("IndexError: string index out of range");
    std::exit(1);
  }

  return PyString::NewInstance(s->buffer() + decoded_subscr,
                               static_cast<int64_t>(1));
}

Handle<PyObject> PyStringKlass::Virtual_Add(Handle<PyObject> self,
                                            Handle<PyObject> other) {
  if (!IsPyString(other)) [[unlikely]] {
    auto other_klass = PyObject::GetKlass(other);
    std::printf("TypeError: can only concatenate str (not \"%.*s\") to str",
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
