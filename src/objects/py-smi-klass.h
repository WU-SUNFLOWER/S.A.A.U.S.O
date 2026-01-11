// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_SMI_KLASS_H_
#define SAAUSO_OBJECTS_PY_SMI_KLASS_H_

#include "src/handles/handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class PyObject;
class PyBoolean;

class PySmiKlass : public Klass {
 public:
  static Tagged<PySmiKlass> GetInstance();

  void Initialize();
  void Finalize();

 private:
  PySmiKlass();
  static Tagged<PySmiKlass> instance_;

  static void Virtual_Print(Handle<PyObject>);

  static Handle<PyObject> Virtual_Add(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Sub(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Mul(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Div(Handle<PyObject>, Handle<PyObject>);
  static Handle<PyObject> Virtual_Mod(Handle<PyObject>, Handle<PyObject>);

  static uint64_t Virtual_Hash(Handle<PyObject> self);

  static Tagged<PyBoolean> Virtual_Greater(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_Less(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_Equal(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_NotEqual(Handle<PyObject>, Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_GreaterEqual(Handle<PyObject>,
                                                Handle<PyObject>);
  static Tagged<PyBoolean> Virtual_LessEqual(Handle<PyObject>,
                                             Handle<PyObject>);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_SMI_KLASS_H_
