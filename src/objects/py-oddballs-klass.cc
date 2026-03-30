// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-oddballs-klass.h"

#include <cstdio>

#include "include/saauso-maybe.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/handles/tagged.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/string-table.h"
#include "src/utils/utils.h"

namespace saauso::internal {

///////////////////////////////////////////////////////////////
// PyBooleanKlass

// static
Tagged<PyBooleanKlass> PyBooleanKlass::GetInstance(Isolate* isolate) {
  Tagged<PyBooleanKlass> instance = isolate->py_boolean_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyBooleanKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_boolean_klass(instance);
  }
  return instance;
}

void PyBooleanKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);
  set_native_layout_kind(NativeLayoutKind::kBoolean);
  set_native_layout_base(PyObjectKlass::GetInstance(isolate));

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.hash_ = &Virtual_Hash;
}

Maybe<void> PyBooleanKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化klass_properties
  set_klass_properties(PyDict::New(isolate));

  // 设置父类并计算mro序列
  AddSuper(PySmiKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 设置类名
  set_name(PyString::New(isolate, "bool"));

  return JustVoid();
}

// static
void PyBooleanKlass::Finalize(Isolate* isolate) {
  isolate->set_py_boolean_klass(Tagged<PyBooleanKlass>::null());
}

// static
// static
Maybe<bool> PyBooleanKlass::Virtual_Equal(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other) {
  bool v = Handle<PyBoolean>::cast(self)->value();

  bool result = false;
  if (IsPySmi(other)) {
    result = PySmi::ToInt(Handle<PySmi>::cast(other)) == v;
  } else if (IsPyFloat(other)) {
    result = Handle<PyFloat>::cast(other)->value() == v;
  } else if (IsPyBoolean(other)) {
    result = Handle<PyBoolean>::cast(other)->value() == v;
  }

  return Maybe<bool>(result);
}

MaybeHandle<PyObject> PyBooleanKlass::Virtual_Repr(Isolate* isolate,
                                                   Handle<PyObject> self) {
  bool v = Handle<PyBoolean>::cast(self)->value();
  return v ? ST(true_symbol, isolate) : ST(false_symbol, isolate);
}

MaybeHandle<PyObject> PyBooleanKlass::Virtual_Str(Isolate* isolate,
                                                  Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

// static
Maybe<bool> PyBooleanKlass::Virtual_NotEqual(Isolate* isolate,
                                             Handle<PyObject> self,
                                             Handle<PyObject> other) {
  bool is_equal;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                             Virtual_Equal(isolate, self, other));

  return Maybe<bool>(!is_equal);
}

// static
Maybe<uint64_t> PyBooleanKlass::Virtual_Hash(Isolate* isolate,
                                             Handle<PyObject> self) {
  return Maybe<uint64_t>(Handle<PyBoolean>::cast(self)->value());
}

///////////////////////////////////////////////////////////////
// PyNoneKlass

// static
Tagged<PyNoneKlass> PyNoneKlass::GetInstance(Isolate* isolate) {
  Tagged<PyNoneKlass> instance = isolate->py_none_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyNoneKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_none_klass(instance);
  }
  return instance;
}

void PyNoneKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 实例对象不创建__dict__字典
  set_instance_has_properties_dict(false);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.repr_ = &Virtual_Repr;
  vtable_.str_ = &Virtual_Str;
  vtable_.equal_ = &Virtual_Equal;
  vtable_.not_equal_ = &Virtual_NotEqual;
  vtable_.hash_ = &Virtual_Hash;
}

Maybe<void> PyNoneKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::New(isolate));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  // 设置类名
  set_name(PyString::New(isolate, "NoneType"));

  return JustVoid();
}

void PyNoneKlass::Finalize(Isolate* isolate) {
  isolate->set_py_none_klass(Tagged<PyNoneKlass>::null());
}

// static
Maybe<bool> PyNoneKlass::Virtual_Equal(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> other) {
  assert(IsPyNone(self, isolate));
  return Maybe<bool>(self.is_identical_to(other));
}

MaybeHandle<PyObject> PyNoneKlass::Virtual_Repr(Isolate* isolate,
                                                Handle<PyObject> self) {
  return ST(none_symbol, isolate);
}

MaybeHandle<PyObject> PyNoneKlass::Virtual_Str(Isolate* isolate,
                                               Handle<PyObject> self) {
  return Virtual_Repr(isolate, self);
}

// static
Maybe<bool> PyNoneKlass::Virtual_NotEqual(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other) {
  bool is_equal;
  ASSIGN_RETURN_ON_EXCEPTION(isolate, is_equal,
                             Virtual_Equal(isolate, self, other));
  return Maybe<bool>(!is_equal);
}

// static
Maybe<uint64_t> PyNoneKlass::Virtual_Hash(Isolate* isolate,
                                          Handle<PyObject> self) {
  assert(IsPyNone(self, isolate));
  return Maybe<uint64_t>((*self).ptr());  // None使用自己的地址作为哈希值
}

}  // namespace saauso::internal
