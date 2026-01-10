// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_UNIVERSE_H_
#define SAAUSO_RUNTIME_UNIVERSE_H_

#include "src/handles/tagged.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Heap;
class Klass;
class PyObject;
class PyBoolean;
class PyNone;
class HandleScopeImplementer;

class Universe {
 public:
  // 基础设施
  static Heap* heap_;
  static HandleScopeImplementer* handle_scope_implementer_;
  static Vector<Klass*> klass_list_;

  // 供VM使用的、永久生存在meta area上的全局单例
  static Tagged<PyNone> py_none_object_;
  static Tagged<PyBoolean> py_true_object_;
  static Tagged<PyBoolean> py_false_object_;

  static void Genesis();
  static void Destroy();

  static Tagged<PyBoolean> ToPyBoolean(bool condition) {
    return condition ? py_true_object_ : py_false_object_;
  }

 private:
  static void InitMetaArea();
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_UNIVERSE_H_
