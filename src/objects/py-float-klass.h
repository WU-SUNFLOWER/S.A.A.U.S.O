// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
#define SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_

#include "objects/klass.h"
#include "py-object.h"

namespace saauso::internal {

class PyFloatKlass : public Klass {
 public:
  static PyFloatKlass* GetInstance();

  void Initialize();

 private:
  PyFloatKlass();
  static PyFloatKlass* instance_;

  static PyObject* Virtual_Add(PyObject*, PyObject*);
  static PyObject* Virtual_Sub(PyObject*, PyObject*);
  static PyObject* Virtual_Mul(PyObject*, PyObject*);
  static PyObject* Virtual_Div(PyObject*, PyObject*);
  static PyObject* Virtual_Mod(PyObject*, PyObject*);

  static PyObject* Virtual_Greater(PyObject*, PyObject*);
  static PyObject* Virtual_Less(PyObject*, PyObject*);
  static PyObject* Virtual_Equal(PyObject*, PyObject*);
  static PyObject* Virtual_NotEqual(PyObject*, PyObject*);
  static PyObject* Virtual_GreaterEqual(PyObject*, PyObject*);
  static PyObject* Virtual_LessEqual(PyObject*, PyObject*);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
