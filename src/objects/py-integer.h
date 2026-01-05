// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "handles/handles.h"
#include "objects/py-object.h"

#ifndef SAAUSO_OBJECTS_PY_INTEGER_H_
#define SAAUSO_OBJECTS_PY_INTEGER_H_

namespace saauso::internal {

class PyInteger : public PyObject {
 public:
  static Handle<PyInteger> NewInstance(int value);

 private:
  int value_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_INTEGER_H_
