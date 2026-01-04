// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_UNIVERSE_H_
#define SAAUSO_RUNTIME_UNIVERSE_H_

#include "objects/object-shape.h"
namespace saauso::internal {

class Heap;
class ObjectShape;
class Klass;

class Universe {
 public:
  static Heap* heap_;

  static ObjectShape* array_list_shape_;
  static ObjectShape* string_shape_;
  static ObjectShape* hash_table_shape_;

  static void Genesis();
  static void Destroy();
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_UNIVERSE_H_
