// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_LIST_KLASS_H_
#define SAAUSO_OBJECTS_PY_LIST_KLASS_H_

#include "src/handles/handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class PyObject;
class PyBoolean;

class PyListKlass : public Klass {
 public:
  static Tagged<PyListKlass> GetInstance();

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  PyListKlass();

  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static void Virtual_Print(Handle<PyObject> self);

  static Handle<PyObject> Virtual_Add(Handle<PyObject> self,
                                      Handle<PyObject> other);
  static Handle<PyObject> Virtual_Mul(Handle<PyObject> self,
                                      Handle<PyObject> coeff);

  static Handle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                         Handle<PyObject> subscr);
  static void Virtual_StoreSubscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr,
                                  Handle<PyObject> value);
  static void Virtual_DelSubscr(Handle<PyObject> self, Handle<PyObject> subscr);
  static Tagged<PyBoolean> Virtual_Less(Handle<PyObject> self,
                                        Handle<PyObject> other);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Tagged<PyBoolean> Virtual_Contains(Handle<PyObject> self,
                                            Handle<PyObject> target);

  static Tagged<PyBoolean> Virtual_Equal(Handle<PyObject> self,
                                         Handle<PyObject> target);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_LIST_KLASS_H_
