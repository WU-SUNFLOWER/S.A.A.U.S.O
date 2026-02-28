// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-object-klass.h"

#include "include/saauso-internal.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

// static
Tagged<PyObjectKlass> PyObjectKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyObjectKlass> instance = isolate->py_object_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyObjectKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_object_klass(instance);
  }
  return instance;
}

///////////////////////////////////////////////////////////////////////////////

void PyObjectKlass::PreInitialize() {
  // 将自己注册到universe
  Isolate::Current()->klass_list().PushBack(Tagged<Klass>(this));

  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

void PyObjectKlass::Initialize() {
  // 建立与type object的双向绑定
  PyTypeObject::NewInstance()->BindWithKlass(Tagged<Klass>(this));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // Python中object类型之上没有父类。
  // 直接调用OrderSupers会得到一个仅含有自己的mro序列。
  OrderSupers();

  // 设置类名
  set_name(PyString::NewInstance("object"));
}

// static
void PyObjectKlass::Finalize() {
  Isolate::Current()->set_py_object_klass(Tagged<PyObjectKlass>::null());
}

size_t PyObjectKlass::Virtual_InstanceSize(Tagged<PyObject> self) {
  return ObjectSizeAlign(sizeof(PyObject));
}

void PyObjectKlass::Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v) {}

}  // namespace saauso::internal
