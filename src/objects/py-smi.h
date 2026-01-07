// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_SMI_H_
#define SAAUSO_OBJECTS_PY_SMI_H_

#include <cstdint>

#include "src/objects/py-object.h"

namespace saauso::internal {

// PySmi只是一个幻影类！
// 外界永远无法创建一个PySmi实例对象，
// 只能使用PySmi*类型的指针来保存整型值！
class PySmi : public PyObject {
 public:
  PySmi() = delete;
  PySmi(const PySmi&) = delete;

  void* operator new(size_t) = delete;
  void operator delete(void*) = delete;

  static constexpr uintptr_t kSmiTagSize = 1;
  static constexpr uintptr_t kSmiTagMask = 1;
  static constexpr uintptr_t kSmiTag = 1;

  // 从Smi*中提取出整型
  int64_t value();

  // 将一个整型转换成PySmi*
  static PySmi* FromInt(int64_t value);

  // 将裸的PyObject*指针转成PySmi*
  static PySmi* Cast(PyObject* object);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_SMI_H_
