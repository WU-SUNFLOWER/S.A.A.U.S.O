// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
#define SAAUSO_OBJECTS_OBJECT_CHECKERS_H_

#include "src/handles/handles.h"
#include "src/objects/object-type-list.h"

namespace saauso::internal {

class PyObject;

#define DECLARE_PY_TYPE(name) class name;
PY_TYPE_LIST(DECLARE_PY_TYPE)
#undef DECLARE_PY_TYPE

#define DECLARE_PY_CHECKER(name)          \
  bool Is##name(Tagged<PyObject> object); \
  bool Is##name(Handle<PyObject> object);

PY_TYPE_LIST(DECLARE_PY_CHECKER)
DECLARE_PY_CHECKER(NormalPyFunction)
DECLARE_PY_CHECKER(NativePyFunction)
DECLARE_PY_CHECKER(PyTrue)
DECLARE_PY_CHECKER(PyFalse)
DECLARE_PY_CHECKER(HeapObject)
DECLARE_PY_CHECKER(GcAbleObject)
#undef DECLARE_PY_CHECKER

// 支持被用户Python代码显式继承的内建类型 Like 与 Exact 语义约定：
// - IsXxx：Like
//   - 如属于具有特殊内存布局的内建容器，采用 native layout kind 进行判定
//   - 否则采用 klass 全等判定作为 fallback
// - IsXxxExact：Exact（按 klass 全等判定）
#define DECLARE_PY_EXACT_CHECKER(name)           \
  bool Is##name##Exact(Tagged<PyObject> object); \
  bool Is##name##Exact(Handle<PyObject> object);

PY_INHERITABLE_TYPE_IN_HEAP_LIST(DECLARE_PY_EXACT_CHECKER)
#undef DECLARE_PY_EXACT_CHECKER

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
