// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
#define SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_

#include "src/objects/klass.h"
#include "src/objects/py-object.h"

namespace saauso::internal {

class PyFloatKlass : public Klass {
 public:
  static Tagged<PyFloatKlass> GetInstance();

  void Initialize();
  void Finalize();

 private:
  PyFloatKlass();
  static Tagged<PyFloatKlass> instance_;

  static void Virtual_Print(Handle<PyObject>);

  static Handle<PyObject> Virtual_Add(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Sub(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Mul(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Div(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Mod(Handle<PyObject>, Handle<PyObject>);

  static Tagged<PyBoolean> Virtual_Greater(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_Less(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_Equal(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_NotEqual(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_GreaterEqual(Handle<PyObject>,
                                                Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_LessEqual(Handle<PyObject>,
                                             Handle<PyObject>);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
