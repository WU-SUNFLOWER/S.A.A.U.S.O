// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_
#define SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_

#include "include/saauso-maybe.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class ObjectVisitor;

class PyTupleKlass : public Klass {
 public:
  static Tagged<PyTupleKlass> GetInstance();
  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  PyTupleKlass();

  static MaybeHandle<PyObject> Virtual_NewInstance(
      Isolate* isolate,
      Handle<PyTypeObject> receiver_type,
      Handle<PyObject> args,
      Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Virtual_Len(Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Repr(Isolate* isolate,
                                            Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Str(Isolate* isolate,
                                           Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                              Handle<PyObject> subscr);
  static MaybeHandle<PyObject> Virtual_StoreSubscr(Handle<PyObject> self,
                                                   Handle<PyObject> subscr,
                                                   Handle<PyObject> value);
  static MaybeHandle<PyObject> Virtual_DelSubscr(Handle<PyObject> self,
                                                 Handle<PyObject> subscr);
  static Maybe<bool> Virtual_Contains(Handle<PyObject> self,
                                      Handle<PyObject> target);
  static Maybe<bool> Virtual_Equal(Handle<PyObject> self,
                                   Handle<PyObject> other);

  static MaybeHandle<PyObject> Virtual_Iter(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_TUPLE_KLASS_H_
