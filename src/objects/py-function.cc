// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-function.h"

#include "src/handles/handles.h"
#include "src/heap/heap.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-string.h"
#include "src/runtime/universe.h"

namespace saauso::internal {

// static
Handle<PyFunction> PyFunction::NewInstance(Handle<PyCodeObject> code_object) {
  HandleScope scope;
  Handle<PyFunction> object = NewInstanceInternal();

  object->func_code_ = *code_object;
  object->func_name_ = code_object->co_name_;
  object->flags_ = code_object->flags_;

  return object.EscapeFrom(&scope);
}

// static
Handle<PyFunction> PyFunction::NewInstance(NativeFuncPointer native_func,
                                           Handle<PyString> func_name) {
  HandleScope scope;
  Handle<PyFunction> object = NewInstanceInternal();

  object->native_func_ = native_func;
  object->func_name_ = *func_name;

  return object.EscapeFrom(&scope);
}

Handle<PyFunction> PyFunction::NewInstanceInternal() {
  Handle<PyFunction> object(
      Universe::heap_->Allocate<PyFunction>(Heap::AllocationSpace::kNewSpace));

  // 绑定klass
  SetKlass(object, NativeFunctionKlass::GetInstance());

  // 初始化properties
  auto properties = PyDict::NewInstance();
  PyDict::Put(properties, PyString::NewInstance("__dict__"), properties);
  PyObject::SetProperties(*object, *properties);

  // 初始化字段
  object->func_code_ = Tagged<PyObject>::Null();
  object->func_name_ = Tagged<PyObject>::Null();
  object->default_args_ = Tagged<PyObject>::Null();
  object->flags_ = 0;
  object->native_func_ = nullptr;

  return object;
}

// static
Tagged<PyFunction> PyFunction::cast(Tagged<PyObject> object) {
  assert(IsPyFunction(object));
  return Tagged<PyFunction>::cast(object);
}

///////////////////////////////////////////////////////////////////////////////

void PyFunction::set_default_args(Handle<PyList> default_args) {
  if (default_args->IsFull()) {
    return;
  }

  Handle<PyList> copied_default_args =
      PyList::NewInstance(default_args->length());
  for (auto i = 0; i < default_args->length(); ++i) {
    HandleScope socpe;
    copied_default_args->Set(i, default_args->Get(i));
  }
  default_args_ = *copied_default_args;
}

// static
Handle<MethodObject> MethodObject::NewInstance(Handle<PyObject> func,
                                               Handle<PyObject> owner) {
  Handle<MethodObject> object(Universe::heap_->Allocate<MethodObject>(
      Heap::AllocationSpace::kNewSpace));
  object->owner_ = *owner;
  object->func_ = *func;
  PyObject::SetKlass(object, MethodObjectKlass::GetInstance());
  return object;
}

Tagged<MethodObject> MethodObject::cast(Tagged<PyObject> object) {
  assert(IsMethodObject(object));
  return Tagged<MethodObject>::cast(object);
}

}  // namespace saauso::internal
