// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_ISOLATE_KLASS_LIST_H_
#define SAAUSO_RUNTIME_ISOLATE_KLASS_LIST_H_

#define ISOLATE_KLASS_LIST(V)           \
  V(PyObject, PyObjectKlass, py_object) \
  V(PySmi, PySmiKlass, py_smi)          \
  V(PyTypeObject, PyTypeObjectKlass, py_type_object) \
  V(PyString, PyStringKlass, py_string) \
  V(PyFunction, PyFunctionKlass, py_function) \
  V(PyFloat, PyFloatKlass, py_float)    \
  V(PyBoolean, PyBooleanKlass, py_boolean) \
  V(PyNone, PyNoneKlass, py_none)       \
  V(PyCodeObject, PyCodeObjectKlass, py_code_object) \
  V(PyList, PyListKlass, py_list)       \
  V(PyDict, PyDictKlass, py_dict)       \
  V(FixedArray, FixedArrayKlass, fixed_array) \
  V(MethodObject, MethodObjectKlass, method_object) \
  V(NativeFunction, NativeFunctionKlass, native_function)

#endif  // SAAUSO_RUNTIME_ISOLATE_KLASS_LIST_H_

