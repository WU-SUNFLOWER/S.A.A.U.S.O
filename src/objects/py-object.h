// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_OBJECTS_H_
#define SAAUSO_OBJECTS_PY_OBJECTS_H_

#include <cstdio>

#include "objects/objects.h"

namespace saauso::internal {

class PyObject : public Object {
  virtual void print() { printf("This is an object.\n"); }
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_OBJECTS_H_
