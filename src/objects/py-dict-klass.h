// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_DICT_KLASS_H_
#define SAAUSO_OBJECTS_PY_DICT_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyBoolean;
class PyTuple;
class ObjectVisitor;

class PyDictKlass : public Klass {
 public:
  static Tagged<PyDictKlass> GetInstance();

  PyDictKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static void Virtual_Print(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Iter(Handle<PyObject> self);
  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);
  static bool Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_NotEqual(Handle<PyObject> self, Handle<PyObject> other);
  static Handle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                         Handle<PyObject> subscr);
  static void Virtual_StoreSubscr(Handle<PyObject> self,
                                  Handle<PyObject> subscr,
                                  Handle<PyObject> value);
  static void Virtual_DeleteSubscr(Handle<PyObject> self,
                                   Handle<PyObject> subscr);
  static bool Virtual_Contains(Handle<PyObject> self, Handle<PyObject> subscr);

  static Handle<PyObject> Virtual_ConstructInstance(
      Tagged<Klass> klass_self,
      Handle<PyObject> args,
      Handle<PyObject> kwargs);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DICT_KLASS_H_
