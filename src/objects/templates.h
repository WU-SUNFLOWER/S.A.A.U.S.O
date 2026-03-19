// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_TEMPLATES_H_
#define SAAUSO_OBJECTS_TEMPLATES_H_

#include "src/handles/global-handles.h"
#include "src/objects/native-function-helpers.h"
#include "src/objects/py-string.h"

namespace saauso::internal {

class PyTypeObject;

class FunctionTemplateInfo {
 public:
  explicit FunctionTemplateInfo(
      NativeFuncPointer function,
      Handle<PyString> name,
      NativeFuncAccessFlag native_access_flag = NativeFuncAccessFlag::kStatic,
      Handle<PyTypeObject> native_owner_type = Handle<PyTypeObject>::null(),
      Handle<PyObject> closure_data = Handle<PyObject>::null())
      : function_(function),
        name_(name),
        native_access_flag_(native_access_flag),
        native_owner_type_(native_owner_type),
        closure_data_(closure_data) {}

  explicit FunctionTemplateInfo(
      NativeFuncPointerWithClosure function_with_closure,
      Handle<PyString> name,
      Handle<PyObject> closure_data,
      NativeFuncAccessFlag native_access_flag = NativeFuncAccessFlag::kStatic,
      Handle<PyTypeObject> native_owner_type = Handle<PyTypeObject>::null())
      : function_with_closure_(function_with_closure),
        name_(name),
        native_access_flag_(native_access_flag),
        native_owner_type_(native_owner_type),
        closure_data_(closure_data) {}

  NativeFuncPointer function() const { return function_; }
  NativeFuncPointerWithClosure function_with_closure() const {
    return function_with_closure_;
  }
  Handle<PyString> name() const { return name_.Get(); }
  NativeFuncAccessFlag native_access_flag() const {
    return native_access_flag_;
  }
  Handle<PyTypeObject> native_owner_type() const {
    return native_owner_type_.Get();
  }
  Handle<PyObject> closure_data() const { return closure_data_.Get(); }

 private:
  NativeFuncPointer function_{nullptr};
  NativeFuncPointerWithClosure function_with_closure_{nullptr};
  Global<PyString> name_;
  NativeFuncAccessFlag native_access_flag_;
  Global<PyTypeObject> native_owner_type_;
  Global<PyObject> closure_data_;
};

}  // namespace saauso::internal

#endif  // INCLUDE_SAAUSO_NATIVE_FUNCTION_H_
