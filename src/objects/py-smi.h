// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_PY_SMI_H_
#define SAAUSO_OBJECTS_PY_SMI_H_

#include <cstdint>

#include "src/objects/py-object.h"

namespace saauso::internal {

class PyString;

// PySmi只是一个幻影类！
// 外界永远无法创建一个PySmi实例对象，
// 只能使用PySmi*类型的指针来保存整型值！
class PySmi : public PyObject {
 public:
  static constexpr int kSmiValueSize = 63;
  static constexpr int64_t kSmiMinValue = -(int64_t{1} << (kSmiValueSize - 1));
  static constexpr int64_t kSmiMaxValue = -(kSmiMinValue + 1);

  PySmi() = delete;
  PySmi(const PySmi&) = delete;

  void* operator new(size_t) = delete;
  void operator delete(void*) = delete;

  // 判断一个Tagged<PyObject>是不是Smi
  static bool IsSmi(Tagged<PyObject> object);
  static bool IsSmi(Handle<PyObject> object);

  // 提取出裸的int64_t
  static int64_t ToInt(Tagged<PySmi> smi);
  static int64_t ToInt(Handle<PySmi> smi);

  // 将一个整型转换成Tagged<PySmi>
  static Tagged<PySmi> FromInt(int64_t value);

  static Tagged<PySmi> FromPyString(Tagged<PyString> py_string);

  // 将裸的Tagged<PyObject>指针转成Tagged<PySmi>
  static Tagged<PySmi> cast(Tagged<PyObject> object);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_PY_SMI_H_
