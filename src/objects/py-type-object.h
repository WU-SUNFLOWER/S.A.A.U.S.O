// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_TYPE_OBJECTS_
#define SAAUSO_OBJECTS_PY_TYPE_OBJECTS_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyTypeObject : public PyObject {
 public:
  static Handle<PyTypeObject> NewInstance();
  static Tagged<PyTypeObject> cast(Tagged<PyObject> object);

  // 建立与klass的双向绑定
  void BindWithKlass(Tagged<Klass> klass);
  Tagged<Klass> own_klass() const { return own_klass_; };

  Handle<PyList> mro() const;

 private:
  friend class PyTypeObjectKlass;

  Tagged<Klass> own_klass_;
};

}  // namespace saauso::internal

#endif
