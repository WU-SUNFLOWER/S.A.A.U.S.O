// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_MODULE_H_
#define SAAUSO_OBJECTS_PY_MODULE_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyModule : public PyObject {
 public:
  // 创建一个新的 module 对象实例，并初始化其 __dict__（properties_）。
  static Handle<PyModule> NewInstance();

  static Tagged<PyModule> cast(Tagged<PyObject> object);

 private:
  friend class PyModuleKlass;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_MODULE_H_

