// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-module-klass.h"

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-module.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"

namespace saauso::internal {

// static
Tagged<PyModuleKlass> PyModuleKlass::GetInstance(Isolate* isolate) {
  Tagged<PyModuleKlass> instance = isolate->py_module_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyModuleKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_module_klass(instance);
  }
  return instance;
}

void PyModuleKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));
  set_instance_has_properties_dict(true);

  // 实例对象创建__dict__字典
  set_instance_has_properties_dict(true);

  // 初始化虚函数表
  vtable_.Clear();
  vtable_.instance_size_ = &Virtual_InstanceSize;
  vtable_.iterate_ = &Virtual_Iterate;
}

Maybe<void> PyModuleKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  // 初始化类字典
  set_klass_properties(PyDict::NewInstance());

  // 设置父类并计算mro序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  // 根据继承关系填充虚函数表
  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  set_name(PyString::NewInstance("module"));

  return JustVoid();
}

void PyModuleKlass::Finalize(Isolate* isolate) {
  isolate->set_py_module_klass(Tagged<PyModuleKlass>::null());
}

size_t PyModuleKlass::Virtual_InstanceSize(Tagged<PyObject>) {
  return ObjectSizeAlign(sizeof(PyModule));
}

void PyModuleKlass::Virtual_Iterate(Tagged<PyObject>, ObjectVisitor*) {}

}  // namespace saauso::internal
