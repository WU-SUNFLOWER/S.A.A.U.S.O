// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_
#define SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyBoolean;

class PyBooleanKlass : public Klass {
 public:
  static PyBooleanKlass* GetInstance();
  void Initialize();

 private:
  PyBooleanKlass();
  static PyBooleanKlass* instance_;

  static void Virtual_Print(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other);
  static Tagged<PyBoolean> Virtual_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other);
};

class PyNoneKlass : public Klass {
 public:
  static PyNoneKlass* GetInstance();
  void Initialize();

 private:
  PyNoneKlass();
  static PyNoneKlass* instance_;

  static void Virtual_Print(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> other);
  static Tagged<PyBoolean> Virtual_NotEqual(Handle<PyObject> self,
                                            Handle<PyObject> other);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_
