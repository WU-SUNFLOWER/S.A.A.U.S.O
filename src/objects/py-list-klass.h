// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_LIST_KLASS_H_
#define SAAUSO_OBJECTS_PY_LIST_KLASS_H_

#include "objects/klass.h"
#include "py-object.h"

namespace saauso::internal {

class PyListKlass : public Klass {
 public:
  static PyListKlass* GetInstance();

  void Initialize();

 private:
  PyListKlass();
  static PyListKlass* instance_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_LIST_KLASS_H_
