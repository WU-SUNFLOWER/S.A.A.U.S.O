// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_HEAP_OBJECT_H_
#define SAAUSO_OBJECTS_HEAP_OBJECT_H_

#include "objects/objects.h"

namespace saauso::internal {

class ObjectShape;

class HeapObject : public Object {
 public:
  ObjectShape* shape() { return shape_; }
  void set_shape(ObjectShape* shape) { shape_ = shape; }

 private:
  ObjectShape* shape_;
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_HEAP_OBJECT_H_
