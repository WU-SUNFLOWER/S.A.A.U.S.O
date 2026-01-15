// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FUNCTION_H_
#define SAAUSO_OBJECTS_PY_FUNCTION_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

using PyFuncFlags = uint32_t;

using NativeFuncPointer = Handle<PyObject> (*)(Handle<PyList> args,
                                               Handle<PyDict> kwargs);

class PyFunction : public PyObject {
 public:
  // 创建python层面的函数对象
  static Handle<PyFunction> NewInstance(Handle<PyCodeObject> code_object);
  // 创建调用c++ native代码的函数对象
  static Handle<PyFunction> NewInstance(NativeFuncPointer native_func,
                                        Handle<PyString> func_name);
  static Tagged<PyFunction> cast(Tagged<PyObject> object);

  Handle<PyString> func_name() const {
    return handle(Tagged<PyString>::cast(func_name_));
  }
  PyFuncFlags flags() const { return flags_; }

  void set_default_args(Handle<PyList> default_args);
  Handle<PyList> default_args() const {
    return handle(Tagged<PyList>::cast(default_args_));
  }

 private:
  friend class PyFunctionKlass;
  friend class NativeFunctionKlass;

  // PyCodeObject* func_code_
  Tagged<PyObject> func_code_{kNullAddress};
  // PyString* func_name_
  Tagged<PyObject> func_name_{kNullAddress};

  // PyList* default_args_
  Tagged<PyObject> default_args_{kNullAddress};

  PyFuncFlags flags_{0};

  NativeFuncPointer native_func_{nullptr};
};

class MethodObject : public PyObject {
 public:
  static Handle<MethodObject> NewInstance(Handle<PyFunction> func,
                                          Handle<PyObject> owner);
  static Tagged<MethodObject> cast(Tagged<PyObject> object);

  void set_owner(Handle<PyObject> owner) { owner_ = *owner; }
  Handle<PyObject> owner() { return handle(owner_); }

  Handle<PyFunction> func() { return handle(Tagged<PyFunction>::cast(func_)); }

 private:
  friend class MethodObjectKlass;

  Tagged<PyObject> owner_;
  Tagged<PyObject> func_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FUNCTION_H_
