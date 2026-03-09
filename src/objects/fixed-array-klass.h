// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_FIXED_ARRAY_KLASS_H_
#define SAAUSO_OBJECTS_FIXED_ARRAY_KLASS_H_

#include "src/objects/klass.h"

namespace saauso::internal {

class ObjectVisitor;

class FixedArrayKlass : public Klass {
 public:
  static Tagged<FixedArrayKlass> GetInstance();
  void PreInitialize(Isolate* isolate);
  Maybe<void> Initialize(Isolate* isolate);
  void Finalize(Isolate* isolate);

 private:
  FixedArrayKlass();

  // GC相关接口
  static size_t Virtual_InstanceSize(Tagged<PyObject> self);
  static void Virtual_Iterate(Tagged<PyObject> self, ObjectVisitor* v);
};

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_FIXED_ARRAY_KLASS_H_
