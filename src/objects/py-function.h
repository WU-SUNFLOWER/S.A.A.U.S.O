// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FUNCTION_H_
#define SAAUSO_OBJECTS_PY_FUNCTION_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/native-function-helpers.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

using PyFuncFlags = uint32_t;

class PyFunction : public PyObject {
 public:
  static Tagged<PyFunction> cast(Tagged<PyObject> object);

  Handle<PyCodeObject> func_code() const;

  Handle<PyString> func_name() const;
  void set_func_name(Handle<PyString> name);

  PyFuncFlags flags() const { return flags_; }

  Handle<PyDict> func_globals() const;
  void set_func_globals(Handle<PyDict> func_globals);
  void set_func_globals(Tagged<PyDict> func_globals);

  void set_default_args(Handle<PyTuple> default_args);
  Handle<PyTuple> default_args() const;

  void set_closures(Handle<PyTuple> closures);
  Handle<PyTuple> closures() const;
  Tagged<PyTuple> closures_tagged() const;

  NativeFuncPointer native_func() const {
    assert(native_func_ != nullptr);
    return native_func_;
  }

 private:
  friend class PyFunctionKlass;
  friend class NativeFunctionKlass;
  friend class Factory;

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
  Tagged<PyObject> closures_;

  PyFuncFlags flags_{0};

  NativeFuncPointer native_func_{nullptr};
};

class MethodObject : public PyObject {
 public:
  static Handle<MethodObject> NewInstance(Handle<PyObject> func,
                                          Handle<PyObject> owner);
  static Tagged<MethodObject> cast(Tagged<PyObject> object);

  void set_owner(Handle<PyObject> owner) { owner_ = *owner; }
  Handle<PyObject> owner() { return handle(owner_); }

  Handle<PyFunction> func() { return handle(Tagged<PyFunction>::cast(func_)); }

 private:
  friend class MethodObjectKlass;
  friend class Factory;

  Tagged<PyObject> owner_;
  Tagged<PyObject> func_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FUNCTION_H_
