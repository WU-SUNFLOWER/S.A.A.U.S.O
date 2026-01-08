// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/py-smi.h"

#include "include/saauso-internal.h"
#include "src/handles/tagged.h"

namespace saauso::internal {

// 判断一个Tagged<PyObject>是不是Smi
bool PySmi::IsSmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

// 从一个Tagged<PySmi>中提取出裸的int64_t
int64_t PySmi::ToInt(Tagged<PySmi> smi) {
  assert(smi.IsSmi());
  return smi.ptr() >> kSmiTagSize;
}

// 将一个整型转换成Tagged<PySmi>
Tagged<PySmi> PySmi::FromInt(int64_t value) {
  return Tagged<PySmi>(SmiToAddress(value));
}

// 将裸的Tagged<PyObject>指针转成Tagged<PySmi>
Tagged<PySmi> PySmi::Cast(Tagged<PyObject> object) {
  assert(object.IsSmi());
  return Tagged<PySmi>(object.ptr());
}

}  // namespace saauso::internal
