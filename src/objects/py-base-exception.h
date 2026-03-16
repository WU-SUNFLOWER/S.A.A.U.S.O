// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_BASE_EXCEPTION_H_
#define SAAUSO_OBJECTS_PY_BASE_EXCEPTION_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyBaseException : public PyObject {
 public:
  static Tagged<PyBaseException> cast(Tagged<PyObject> object);

 private:
  friend class PyBaseExceptionKlass;
  friend class Factory;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_BASE_EXCEPTION_H_
