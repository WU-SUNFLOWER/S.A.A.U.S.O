// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-type-object-klass.h"

#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

Tagged<PyTypeObjectKlass> PyTypeObjectKlass::instance_(nullptr);

///////////////////////////////////////////////////////////////
// PyTypeObjectKlass

// static
Tagged<PyTypeObjectKlass> PyTypeObjectKlass::GetInstance() {
  if (instance_.IsNull()) [[unlikely]] {
    instance_ = Universe::heap_->Allocate<PyTypeObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
  }
  return instance_;
}

void PyTypeObjectKlass::Initialize() {
  // 将自己注册到universe
  Universe::klass_list_.PushBack(this);

  // TODO: 初始化虚函数表
  vtable_.print = &Virtual_Print;
  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;

  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance());
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("type"));
}

void PyTypeObjectKlass::Finalize() {
  instance_ = Tagged<PyTypeObjectKlass>::Null();
}

///////////////////////////////////////////////////////////////////////////

// static
void PyTypeObjectKlass::Virtual_Print(Handle<PyObject> self) {
  auto type_object = Handle<PyTypeObject>::cast(self);
  auto type_name = type_object->own_klass()->name();
  std::printf("<class '%.*s'>", static_cast<int>(type_name->length()),
              type_name->buffer());
}

Handle<PyObject> PyTypeObjectKlass::Virtual_GetAttr(
    Handle<PyObject> self,
    Handle<PyObject> prop_name) {
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();
  return own_klass->klass_properties()->Get(prop_name);
}

void PyTypeObjectKlass::Virtual_SetAttr(Handle<PyObject> self,
                                        Handle<PyObject> prop_name,
                                        Handle<PyObject> prop_value) {
  auto own_klass = Handle<PyTypeObject>::cast(self)->own_klass();
  PyDict::Put(own_klass->klass_properties(), prop_name, prop_value);
}

// static
size_t PyTypeObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return sizeof(PyTypeObject);
}

// static
void PyTypeObjectKlass::Virtual_Iterate(Tagged<PyObject> self,
                                        ObjectVisitor* v) {
  // do nothing
}

}  // namespace saauso::internal
