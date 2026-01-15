// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-function-klass.h"

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

//////////////////////////////////////////////////////////////////////////
// PyFunctionKlass实现

Tagged<PyFunctionKlass> PyFunctionKlass::instance_(kNullAddress);

Tagged<PyFunctionKlass> PyFunctionKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyFunctionKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyFunctionKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyFunctionKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("function"));
}

void PyFunctionKlass::Finalize() {
  instance_ = Tagged<PyFunctionKlass>::Null();
}

void PyFunctionKlass::Virtual_Print(Handle<PyObject> self) {
  // <function f at 0x000001C4EF73D280>
  auto func = Handle<PyFunction>::cast(self);
  std::printf("<function ");
  PyObject::Print(func->func_name());
  std::printf(" at 0x%p>", reinterpret_cast<void*>((*func).ptr()));
}

size_t PyFunctionKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyFunction);
}

void PyFunctionKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {
  auto func = Tagged<PyFunction>::cast(self);
  v->VisitPointer(&func->func_code_);
  v->VisitPointer(&func->func_name_);
  v->VisitPointer(&func->default_args_);
}

///////////////////////////////////////////////////////////////
// NativeFunctionKlass

Tagged<NativeFunctionKlass> NativeFunctionKlass::instance_(kNullAddress);

Tagged<NativeFunctionKlass> NativeFunctionKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<NativeFunctionKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void NativeFunctionKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.call = &Virtual_Call;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void NativeFunctionKlass::Initialize() {}

void NativeFunctionKlass::Finalize() {
  instance_ = Tagged<NativeFunctionKlass>::Null();
}

void NativeFunctionKlass::Virtual_Print(Handle<PyObject> self) {
  // <built-in function len>
  auto func = Handle<PyFunction>::cast(self);
  std::printf("<built-in function ");
  PyObject::Print(func->func_name());
  std::printf(">");
}

Handle<PyObject> NativeFunctionKlass::Virtual_Call(Handle<PyObject> self,
                                                   Handle<PyObject> args,
                                                   Handle<PyObject> kwargs) {
  auto func = Handle<PyFunction>::cast(self);
  return func->native_func_(Handle<PyList>::cast(args),
                            Handle<PyDict>::cast(kwargs));
}

size_t NativeFunctionKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyFunction);
}

void NativeFunctionKlass::Virtual_Iterate(Tagged<PyObject> self,
                                          ObjectVisitor* v) {
  auto func = Tagged<PyFunction>::cast(self);
  v->VisitPointer(&func->func_name_);
  v->VisitPointer(&func->default_args_);
  v->VisitPointer(&func->func_code_);
}

///////////////////////////////////////////////////////////////
// MethodKlass

Tagged<MethodObjectKlass> MethodObjectKlass::instance_(kNullAddress);

Tagged<MethodObjectKlass> MethodObjectKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<MethodObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void MethodObjectKlass::PreInitialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void MethodObjectKlass::Initialize() {}

void MethodObjectKlass::Finalize() {
  instance_ = Tagged<MethodObjectKlass>::Null();
}

void MethodObjectKlass::Virtual_Print(Handle<PyObject> self) {
  // TODO
}

size_t MethodObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(MethodObject);
}

void MethodObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  auto object = Tagged<MethodObject>::cast(self);
  // TODO: 完善iterate
  v->VisitPointer(&object->func_);
  v->VisitPointer(&object->owner_);
}

}  // namespace saauso::internal
