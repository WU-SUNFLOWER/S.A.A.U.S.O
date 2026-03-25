// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_STRING_KLASS_H_
#define SAAUSO_OBJECTS_PY_STRING_KLASS_H_

#include "include/saauso-maybe.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"

namespace saauso::internal {

class PyObject;
class PyBoolean;

class PyStringKlass : public Klass {
 public:
  static Tagged<PyStringKlass> GetInstance(Isolate* isolate);

  PyStringKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
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

  static Maybe<bool> Virtual_Equal(Isolate* isolate,
                                   Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Maybe<bool> Virtual_NotEqual(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> other);
  static Maybe<bool> Virtual_Less(Isolate* isolate,
                                  Handle<PyObject> self,
                                  Handle<PyObject> other);
  static Maybe<bool> Virtual_Greater(Isolate* isolate,
                                     Handle<PyObject> self,
                                     Handle<PyObject> other);
  static Maybe<bool> Virtual_LessEqual(Isolate* isolate,
                                       Handle<PyObject> self,
                                       Handle<PyObject> other);
  static Maybe<bool> Virtual_GreaterEqual(Isolate* isolate,
                                          Handle<PyObject> self,
                                          Handle<PyObject> other);

  static Maybe<bool> Virtual_Contains(Isolate* isolate,
                                      Handle<PyObject> self,
                                      Handle<PyObject> target);

  static MaybeHandle<PyObject> Virtual_Subscr(Handle<PyObject> self,
                                              Handle<PyObject> subscr);
  static MaybeHandle<PyObject> Virtual_Add(Isolate* isolate,
                                           Handle<PyObject> self,
                                           Handle<PyObject> other);
  static Maybe<uint64_t> Virtual_Hash(Isolate* isolate, Handle<PyObject> self);

  // GC相关接口
  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_STRING_KLASS_H_
