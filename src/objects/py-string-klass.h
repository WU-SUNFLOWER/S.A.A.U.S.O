// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_STRING_KLASS_H_
#define SAAUSO_OBJECTS_PY_STRING_KLASS_H_

#include "objects/klass.h"
#include "py-object.h"

namespace saauso::internal {

class PyStringKlass : public Klass {
 public:
  static PyStringKlass* GetInstance();

  void Initialize();

 private:
  PyStringKlass();
  static PyStringKlass* instance_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_STRING_KLASS_H_
