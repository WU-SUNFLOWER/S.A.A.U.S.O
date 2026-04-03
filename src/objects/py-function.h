// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FUNCTION_H_
#define SAAUSO_OBJECTS_PY_FUNCTION_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/native-function-helpers.h"
#include "src/objects/py-object.h"
#include "src/objects/py-type-object.h"

namespace saauso::internal {

class PyFunction : public PyObject {
 public:
  static Tagged<PyFunction> cast(Tagged<PyObject> object);

  Handle<PyCodeObject> func_code(Isolate* isolate) const;
  void set_func_code(Handle<PyCodeObject> code);
  void set_func_code(Tagged<PyCodeObject> code);

  Handle<PyString> func_name(Isolate* isolate) const;
  void set_func_name(Handle<PyString> name);
  void set_func_name(Tagged<PyString> name);

  Handle<PyDict> func_globals(Isolate* isolate) const;
  void set_func_globals(Handle<PyDict> func_globals);
  void set_func_globals(Tagged<PyDict> func_globals);

  void set_default_args(Handle<PyTuple> default_args);
  void set_default_args(Tagged<PyTuple> default_args);
  Handle<PyTuple> default_args(Isolate* isolate) const;

  void set_closures(Handle<PyTuple> closures);
  void set_closures(Tagged<PyTuple> closures);
  Handle<PyTuple> closures(Isolate* isolate) const;
  Tagged<PyTuple> closures_tagged() const;

  NativeFuncPointer native_func() const { return native_func_; }
  void set_native_func(NativeFuncPointer native_func) {
    native_func_ = native_func;
  }

  NativeFuncPointerWithClosure native_func_with_closure() const {
    return native_func_with_closure_;
  }
  void set_native_func_with_closure(NativeFuncPointerWithClosure native_func) {
    native_func_with_closure_ = native_func;
  }
  bool has_closure_native_func() const {
    return native_func_with_closure_ != nullptr;
  }

  Handle<PyObject> native_closure_data(Isolate* isolate) const {
    return handle(native_closure_data_, isolate);
  }
  void set_native_closure_data(Handle<PyObject> closure_data);
  void set_native_closure_data(Tagged<PyObject> closure_data);

  NativeFuncAccessFlag native_access_flag() const {
    return native_access_flag_;
  }
  void set_native_access_flag(NativeFuncAccessFlag kind) {
    native_access_flag_ = kind;
  }
  bool is_native_instance_method() const {
    return native_access_flag_ == NativeFuncAccessFlag::kInstanceMethod;
  }
  Handle<PyTypeObject> native_owner_type(Isolate* isolate) const {
    return handle(Tagged<PyTypeObject>::cast(native_owner_type_), isolate);
  }
  void set_native_owner_type(Handle<PyTypeObject> owner_type);
  void set_native_owner_type(Tagged<PyTypeObject> owner_type);

 private:
  friend class PyFunctionKlass;
  friend class NativeFunctionKlass;

  static Handle<PyFunction> NewInstanceInternal();

  // PyCodeObject* func_code_
  Tagged<PyObject> func_code_{kNullAddress};
  // PyString* func_name_
  Tagged<PyObject> func_name_{kNullAddress};

  // PyDict* func_globals_
  // 与函数绑定的全局变量表，在函数被创建时确定
  Tagged<PyObject> func_globals_{kNullAddress};

  // PyTuple* default_args_
  Tagged<PyObject> default_args_{kNullAddress};

  // PyTuple* closures_
  Tagged<PyObject> closures_{kNullAddress};

  NativeFuncPointer native_func_{nullptr};
  NativeFuncPointerWithClosure native_func_with_closure_{nullptr};
  Tagged<PyObject> native_closure_data_{kNullAddress};
  NativeFuncAccessFlag native_access_flag_{NativeFuncAccessFlag::kStatic};
  // PyTypeObject* native_owner_type_
  Tagged<PyObject> native_owner_type_{kNullAddress};
};

class MethodObject : public PyObject {
 public:
  static Handle<MethodObject> NewInstance(Handle<PyObject> func,
                                          Handle<PyObject> owner);
  static Tagged<MethodObject> cast(Tagged<PyObject> object);

  void set_owner(Handle<PyObject> owner);
  void set_owner(Tagged<PyObject> owner);
  Handle<PyObject> owner(Isolate* isolate) const {
    return handle(owner_, isolate);
  }

  void set_func(Handle<PyObject> func);
  void set_func(Tagged<PyObject> func);
  Handle<PyFunction> func(Isolate* isolate) const {
    return handle(Tagged<PyFunction>::cast(func_), isolate);
  }

 private:
  friend class MethodObjectKlass;

  Tagged<PyObject> owner_;
  Tagged<PyObject> func_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FUNCTION_H_
