// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "handles/handles.h"
#include "objects/py-object.h"

#ifndef SAAUSO_OBJECTS_PY_FLOAT_H_
#define SAAUSO_OBJECTS_PY_FLOAT_H_

namespace saauso::internal {

class PyFloat : public PyObject {
 public:
  static Handle<PyFloat> NewInstance(double value);

  static PyFloat* Cast(PyObject* object);

  double value() const { return value_; }
  void set_value(double value) { value_ = value; }

 private:
  double value_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_DOUBLE_H_
