// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_
#define SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_

#include "src/handles/maybe-handles.h"
#include "src/objects/klass.h"
#include "src/utils/maybe.h"

namespace saauso::internal {

class PyBoolean;

class PyBooleanKlass : public Klass {
 public:
  static Tagged<PyBooleanKlass> GetInstance();

  PyBooleanKlass() = delete;

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_Repr(Isolate* isolate,
                                            Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Str(Isolate* isolate,
                                           Handle<PyObject> self);
  static Maybe<bool> Virtual_Equal(Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Maybe<bool> Virtual_NotEqual(Handle<PyObject> self,
                                      Handle<PyObject> other);
  static Maybe<uint64_t> Virtual_Hash(Handle<PyObject> self);
};

class PyNoneKlass : public Klass {
 public:
  static Tagged<PyNoneKlass> GetInstance();
  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  PyNoneKlass();

  static MaybeHandle<PyObject> Virtual_Repr(Isolate* isolate,
                                            Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Str(Isolate* isolate,
                                           Handle<PyObject> self);
  static Maybe<bool> Virtual_Equal(Handle<PyObject> self,
                                   Handle<PyObject> other);
  static Maybe<bool> Virtual_NotEqual(Handle<PyObject> self,
                                      Handle<PyObject> other);
  static Maybe<uint64_t> Virtual_Hash(Handle<PyObject> self);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_BOOLEAN_KLASS_H_
