// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-base-exception-klass.h"

#include "include/saauso-internal.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/heap/heap.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"

namespace saauso::internal {

// static
Tagged<PyBaseExceptionKlass> PyBaseExceptionKlass::GetInstance() {
  Isolate* isolate = Isolate::Current();
  Tagged<PyBaseExceptionKlass> instance = isolate->py_base_exception_klass();
  if (instance.is_null()) [[unlikely]] {
    instance = isolate->heap()->Allocate<PyBaseExceptionKlass>(
        Heap::AllocationSpace::kMetaSpace);
    isolate->set_py_base_exception_klass(instance);
  }
  return instance;
}

void PyBaseExceptionKlass::PreInitialize(Isolate* isolate) {
  isolate->klass_list().PushBack(Tagged<Klass>(this));

  // BaseException 实例目前仍采用 __dict__ 存储语义字段（args/message 等）。
  set_instance_has_properties_dict(true);
  set_native_layout_kind(NativeLayoutKind::kPyObject);
  set_native_layout_base(PyObjectKlass::GetInstance());

  vtable_.Clear();
}

Maybe<void> PyBaseExceptionKlass::Initialize(Isolate* isolate) {
  RETURN_ON_EXCEPTION(isolate, CreateAndBindToPyTypeObject(isolate));

  Handle<PyDict> klass_properties = PyDict::NewInstance();
  set_klass_properties(klass_properties);
  set_name(PyString::NewInstance("BaseException"));

  AddSuper(PyObjectKlass::GetInstance());
  RETURN_ON_EXCEPTION(isolate, OrderSupers(isolate));

  RETURN_ON_EXCEPTION(isolate,
                      vtable_.Initialize(isolate, Tagged<Klass>(this)));

  return JustVoid();
}

void PyBaseExceptionKlass::Finalize(Isolate* isolate) {
  isolate->set_py_base_exception_klass(Tagged<PyBaseExceptionKlass>::null());
}

}  // namespace saauso::internal
