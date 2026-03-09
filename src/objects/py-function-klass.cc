// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-function-klass.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

//////////////////////////////////////////////////////////////////////////
// PyFunctionKlass实现

Tagged<PyFunctionKlass> PyFunctionKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyFunctionKlass> instance = isolate->py_function_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyFunctionKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_function_klass(instance);
  }
  return instance;
}

void PyFunctionKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print_ = &Virtual_Print;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyFunctionKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 设置类名
  set_name(PyString::NewInstance("function"));

  return JustVoid();
}

void PyFunctionKlass::Finalize(Isolate* isolate) {
  isolate->set_py_function_klass(Tagged<PyFunctionKlass>::null());
}

MaybeHandle<PyObject> PyFunctionKlass::Virtual_Print(Handle<PyObject> self) {
  auto func = Handle<PyFunction>::cast(self);
  std::printf("<function ");
  if (PyObject::Print(func->func_name()).IsEmpty()) {
    return kNullMaybeHandle;
  }
  std::printf(" at 0x%p>", reinterpret_cast<void*>((*func).ptr()));
  return handle(Isolate::Current()->py_none_object());
}

size_t PyFunctionKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyFunction));
}

void PyFunctionKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  auto func = Tagged<PyFunction>::cast(self);
  v->VisitPointer(&func->func_code_);
  v->VisitPointer(&func->func_name_);
  v->VisitPointer(&func->func_globals_);
  v->VisitPointer(&func->default_args_);
  v->VisitPointer(&func->closures_);
}

///////////////////////////////////////////////////////////////
// NativeFunctionKlass

Tagged<NativeFunctionKlass> NativeFunctionKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<NativeFunctionKlass> instance = isolate->native_function_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<NativeFunctionKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_native_function_klass(instance);
  }
  return instance;
}

void NativeFunctionKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print_ = &Virtual_Print;
  vtable_.call_ = &Virtual_Call;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> NativeFunctionKlass::Initialize(Isolate* isolate) {
  return JustVoid();
}

void NativeFunctionKlass::Finalize(Isolate* isolate) {
  isolate->set_native_function_klass(Tagged<NativeFunctionKlass>::null());
}

MaybeHandle<PyObject> NativeFunctionKlass::Virtual_Print(
    Handle<PyObject> self) {
  auto func = Handle<PyFunction>::cast(self);
  std::printf("<built-in function ");
  if (PyObject::Print(func->func_name()).IsEmpty()) {
    return kNullMaybeHandle;
  }
  std::printf(">");
  return handle(Isolate::Current()->py_none_object());
}

MaybeHandle<PyObject> NativeFunctionKlass::Virtual_Call(
    Isolate* isolate,
    Handle<PyObject> self,
    Handle<PyObject> host,
    Handle<PyObject> args,
    Handle<PyObject> kwargs) {
  assert(IsNativePyFunction(self));
  auto func = Handle<PyFunction>::cast(self);
  return func->native_func_(host, Handle<PyTuple>::cast(args),
                            Handle<PyDict>::cast(kwargs));
}

size_t NativeFunctionKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyFunction));
}

void NativeFunctionKlass::Virtual_Iterate(Tagged<PyObject> self,
                                          ObjectVisitor* v) {
  auto func = Tagged<PyFunction>::cast(self);
  v->VisitPointer(&func->func_name_);
  v->VisitPointer(&func->default_args_);
  v->VisitPointer(&func->func_code_);
  v->VisitPointer(&func->func_globals_);
  v->VisitPointer(&func->closures_);
}

///////////////////////////////////////////////////////////////
// MethodKlass

Tagged<MethodObjectKlass> MethodObjectKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<MethodObjectKlass> instance = isolate->method_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<MethodObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_method_object_klass(instance);
  }
  return instance;
}

void MethodObjectKlass::PreInitialize(Isolate* isolate) {
  // 将自己注册到universe
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // 初始化虚函数表
  vtable_.print_ = &Virtual_Print;
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> MethodObjectKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 设置类名
  set_name(PyString::NewInstance("method"));

  return JustVoid();
}

void MethodObjectKlass::Finalize(Isolate* isolate) {
  isolate->set_method_object_klass(Tagged<MethodObjectKlass>::null());
}

MaybeHandle<PyObject> MethodObjectKlass::Virtual_Print(Handle<PyObject> self) {
  // TODO: 实现method object类型的print方法
  return handle(Isolate::Current()->py_none_object());
}

size_t MethodObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(MethodObject));
}

void MethodObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  auto object = Tagged<MethodObject>::cast(self);
  v->VisitPointer(&object->func_);
  v->VisitPointer(&object->owner_);
}

}  // namespace saauso::internal
