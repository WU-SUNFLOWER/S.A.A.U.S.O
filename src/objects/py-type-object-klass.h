// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TYPE_OBJECT_KLASS_H_
#define SAAUSO_OBJECTS_PY_TYPE_OBJECT_KLASS_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

class ObjectVisitor;

class PyTypeObjectKlass : public Klass {
 public:
  static Tagged<PyTypeObjectKlass> GetInstance();

  PyTypeObjectKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_Print(Handle<PyObject> self);

  static Maybe<bool> Virtual_GetAttr(Handle<PyObject> self,
                                     Handle<PyObject> prop_name,
                                     bool is_try,
                                     Handle<PyObject>& out_prop_val);
  static MaybeHandle<PyObject> Virtual_SetAttr(Handle<PyObject> self,
                                               Handle<PyObject> prop_name,
                                               Handle<PyObject> prop_value);

  static Maybe<uint64_t> Virtual_Hash(Handle<PyObject> self);
  static Maybe<bool> Virtual_Equal(Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Maybe<bool> Virtual_NotEqual(Handle<PyObject> self,
                                      Handle<PyObject> other);

  static MaybeHandle<PyObject> Virtual_NewInstance(
      Isolate* isolate,
      Handle<PyTypeObject> receiver_type,
      Handle<PyObject> args,
      Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Virtual_Call(Isolate* isolate,
                                            Handle<PyObject> self,
                                            Handle<PyObject> host,
                                            Handle<PyObject> args,
                                            Handle<PyObject> kwargs);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif
