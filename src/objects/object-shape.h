// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_OBJECT_SHAPE_H_
#define SAAUSO_OBJECTS_OBJECT_SHAPE_H_

#include "objects/heap-object.h"

namespace saauso::internal {

class Klass;

class ObjectShape : public HeapObject {
 public:
  Klass* klass() { return klass_; }
  void set_klass(Klass* klass) { klass_ = klass; }

 private:
  Klass* klass_{nullptr};

  // TODO(wu-sunflower): ObjectShape作为Hidden Class使用
  // Object* transitions_;   // 用于 Hidden Class 转换树
  // Object* descriptors_;   // 用于描述属性名和 Offset 的映射
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_OBJECT_SHAPE_H_
