// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_STRING_KLASS_H_
#define SAAUSO_OBJECTS_PY_STRING_KLASS_H_

#include "src/handles/handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class PyObject;
class PyBoolean;

class PyStringKlass : public Klass {
 public:
  static Tagged<PyStringKlass> GetInstance();

  PyStringKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static Handle<PyObject> Virtual_Len(Handle<PyObject> self);

  static bool Virtual_Equal(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_NotEqual(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_Less(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_Greater(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_LessEqual(Handle<PyObject> self, Handle<PyObject> other);
  static bool Virtual_GreaterEqual(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static bool Virtual_Contains(Handle<PyObject> self, Handle<PyObject> target);

  static Handle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                         Handle<PyObject> subscr);
  static Handle<PyObject> Virtual_Add(Handle<PyObject> self,
                                      Handle<PyObject> other);
  static void Virtual_Print(Handle<PyObject> self);
  static uint64_t Virtual_Hash(Handle<PyObject> self);

  // GC相关接口
  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_STRING_KLASS_H_
