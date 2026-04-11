// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_
#define SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class Isolate;

class PyBaseExceptionKlass : public Klass {
 public:
  PyBaseExceptionKlass() = delete;

  static Tagged<PyBaseExceptionKlass> GetInstance(Isolate* isolate);

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);

  static MaybeHandle<PyObject> Virtual_InitInstance(Isolate* isolate,
                                                    Handle<PyObject> instance,
                                                    Handle<PyObject> args,
                                                    Handle<PyObject> kwargs);

  static MaybeHandle<PyObject> Virtual_Repr(Isolate* isolate,
                                            Handle<PyObject> self);
  static MaybeHandle<PyObject> Virtual_Str(Isolate* isolate,
                                           Handle<PyObject> self);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_
