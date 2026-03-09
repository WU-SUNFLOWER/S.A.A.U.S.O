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
Tagged<PyModuleKlass> PyModuleKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
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

  vtable_.instance_size = &Virtual_InstanceSize;
  vtable_.iterate = &Virtual_Iterate;
}

Maybe<void> PyModuleKlass::Initialize(Isolate* isolate) {
  // 建立与type object的双向绑定
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  set_klass_properties(PyDict::NewInstance());

  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  set_name(PyString::NewInstance("module"));

  return JustVoid();
}

void PyModuleKlass::Finalize() {
  Isolate::Current()->set_py_module_klass(Tagged<PyModuleKlass>::null());
}

size_t PyModuleKlass::Virtual_InstanceSize(Tagged<PyObject>) {
  return ObjectSizeAlign(sizeof(PyModule));
}

void PyModuleKlass::Virtual_Iterate(Tagged<PyObject>, ObjectVisitor*) {}

}  // namespace saauso::internal
