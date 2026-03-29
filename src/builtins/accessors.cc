// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/builtins/accessors.h"

#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-string.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"

namespace saauso::internal {

//////////////////////////////////////////////////////////////////////////////////////////
// AccessorDescriptor 定义

const AccessorDescriptor Accessors::kPyObjectClassAccessor = {
    .getter = &PyObjectClassGetter,
    .setter = &PyObjectClassSetter};

const AccessorDescriptor Accessors::kPyObjectDictAccessor = {
    .getter = &PyObjectDictGetter,
    .setter = &PyObjectDictSetter};

//////////////////////////////////////////////////////////////////////////////////////////
// Accessor Getter/Setter 函数实现

MaybeHandle<PyObject> Accessors::PyObjectClassGetter(
    Isolate* isolate,
    Handle<PyObject> receiver) {
  return PyObject::GetKlass(receiver)->type_object();
}

MaybeHandle<PyObject> Accessors::PyObjectClassSetter(Isolate* isolate,
                                                     Handle<PyObject> receiver,
                                                     Handle<PyObject> value) {
  Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                     "can't set attribute '__class__'");
  return kNullMaybeHandle;
}

MaybeHandle<PyObject> Accessors::PyObjectDictGetter(Isolate* isolate,
                                                    Handle<PyObject> receiver) {
  Handle<PyDict> properties = PyObject::GetProperties(receiver);
  if (properties.is_null()) {
    Runtime_ThrowErrorf(isolate, ExceptionType::kAttributeError,
                        "'%s' object has no attribute '__dict__'",
                        PyObject::GetTypeName(receiver, isolate)->buffer());
    return kNullMaybeHandle;
  }
  return properties;
}

MaybeHandle<PyObject> Accessors::PyObjectDictSetter(Isolate* isolate,
                                                    Handle<PyObject> receiver,
                                                    Handle<PyObject> value) {
  Runtime_ThrowError(isolate, ExceptionType::kTypeError,
                     "can't set attribute '__dict__'");
  return kNullMaybeHandle;
}

}  // namespace saauso::internal
