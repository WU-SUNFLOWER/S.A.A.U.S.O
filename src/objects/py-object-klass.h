// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_
#define SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class PyObjectKlass : public Klass {
 public:
  static Tagged<PyObjectKlass> GetInstance();

  PyObjectKlass() = delete;

  void Initialize();
  void Finalize();

 private:
  static Tagged<PyObjectKlass> instance_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECT_KLASS_H_
