// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/klass.h"
#include "src/objects/py-object.h"

#ifndef SAAUSO_OBJECTS_PY_ODDBALLS_H_
#define SAAUSO_OBJECTS_PY_ODDBALLS_H_

namespace saauso::internal {

class PyBoolean : public PyObject {
 public:
  static PyBoolean* NewInstance(bool value);
  static PyBoolean* Cast(PyObject* object);

  bool value() const { return value_; }

  PyBoolean* Reverse();

 private:
  bool value_;
};

class PyNone : public PyObject {
 public:
  static PyNone* NewInstance();
  static PyNone* Cast(PyObject* object);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_ODDBALLS_H_
