// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_CODE_OBJECT_KLASS_H_
#define SAAUSO_OBJECTS_PY_CODE_OBJECT_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyCodeObjectKlass : public Klass {
 public:
  static Tagged<PyCodeObjectKlass> GetInstance();

  PyCodeObjectKlass() = delete;

  void PreInitialize();
  void Initialize();
  void Finalize();

 private:
  static void Virtual_Print(Handle<PyObject> self);

  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_CODE_OBJECT_KLASS_H_
