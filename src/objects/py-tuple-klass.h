// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_
#define SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class ObjectVisitor;

class PyTupleKlass : public Klass {
 public:
  static Tagged<PyTupleKlass> GetInstance();
  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  PyTupleKlass();

  static Handle<PyObject> Virtual_ConstructInstance(Tagged<Klass> klass_self,
                                                    Handle<PyObject> args,
                                                    Handle<PyObject> kwargs);

  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                         Handle<PyObject> subscr);
  static void Virtual_StoreSubscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr,
                                  Handle<PyObject> value);
  static void Virtual_DelSubscr(Handle<PyObject> self, Handle<PyObject> subscr);
  static bool Virtual_Contains(Handle<PyObject> self, Handle<PyObject> target);
  static bool Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other);

  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_
