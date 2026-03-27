// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_FIXED_ARRAY_H_
#define SAAUSO_OBJECTS_FIXED_ARRAY_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

class FixedArray : public PyObject {
 public:
  static Tagged<FixedArray> cast(Tagged<PyObject> object);

  static size_t ComputeObjectSize(int64_t capacity);

  // 获取/设置元素 (不做边界检查，追求极致性能，由调用者保证安全)
  Tagged<PyObject> Get(int64_t index);
  void Set(int64_t index, Handle<PyObject> value);
  void Set(int64_t index, Tagged<PyObject> value);

  int64_t capacity() const { return capacity_; }

 private:
  friend class FixedArrayKlass;
  friend class Factory;

  // 获取数据首地址
  Tagged<PyObject>* data() {
    return reinterpret_cast<Tagged<PyObject>*>(this + 1);
  }

  int64_t capacity_{0};
  // 紧接着是 Tagged<PyObject> elements_[capacity_];
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_FIXED_ARRAY_H_
