// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
#define SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"
#include "src/objects/py-object.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

class PyFloatKlass : public Klass {
 public:
  static Tagged<PyFloatKlass> GetInstance();

  void PreInitialize();
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize();

 private:
  PyFloatKlass();

  static MaybeHandle<PyObject> Virtual_ConstructInstance(
      Tagged<Klass> klass_self,
      Handle<PyObject> args,
      Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Virtual_Print(Handle<PyObject>);

  static MaybeHandle<PyObject> Virtual_Add(Handle<PyObject>, Handle<PyObject>);
  static MaybeHandle<PyObject> Virtual_Sub(Handle<PyObject>, Handle<PyObject>);
  static MaybeHandle<PyObject> Virtual_Mul(Handle<PyObject>, Handle<PyObject>);
  static MaybeHandle<PyObject> Virtual_Div(Handle<PyObject>, Handle<PyObject>);
  static MaybeHandle<PyObject> Virtual_FloorDiv(Handle<PyObject>,
                                                  Handle<PyObject>);
  static MaybeHandle<PyObject> Virtual_Mod(Handle<PyObject>, Handle<PyObject>);

  static Maybe<bool> Virtual_Greater(Handle<PyObject>, Handle<PyObject>);
  static Maybe<bool> Virtual_Less(Handle<PyObject>, Handle<PyObject>);
  static Maybe<bool> Virtual_Equal(Handle<PyObject>, Handle<PyObject>);
  static Maybe<bool> Virtual_NotEqual(Handle<PyObject>, Handle<PyObject>);
  static Maybe<bool> Virtual_GreaterEqual(Handle<PyObject>, Handle<PyObject>);
  static Maybe<bool> Virtual_LessEqual(Handle<PyObject>, Handle<PyObject>);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_FLOAT_KLASS_H_
