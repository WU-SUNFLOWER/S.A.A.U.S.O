// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_
#define SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyBaseExceptionKlass : public Klass {
 public:
  PyBaseExceptionKlass() = delete;

  static Tagged<PyBaseExceptionKlass> GetInstance();

  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  static MaybeHandle<PyObject> Virtual_InitInstance(Isolate* isolate,
                                                    Handle<PyObject> instance,
                                                    Handle<PyObject> args,
                                                    Handle<PyObject> kwargs);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_BASE_EXCEPTION_KLASS_H_
