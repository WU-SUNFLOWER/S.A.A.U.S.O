// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
#define SAAUSO_OBJECTS_OBJECT_CHECKERS_H_

#include "src/handles/handles.h"
#include "src/objects/object-type-list.h"

namespace saauso::internal {

class Isolate;
class PyObject;

#define DECLARE_PY_TYPE(name) class name;
PY_TYPE_LIST(DECLARE_PY_TYPE)
#undef DECLARE_PY_TYPE

#define DECLARE_PY_CHECKER(name)          \
  bool Is##name(Tagged<PyObject> object); \
  bool Is##name(Handle<PyObject> object);

DECLARE_PY_CHECKER(PySmi)
DECLARE_PY_CHECKER(PyString)
DECLARE_PY_CHECKER(PyList)
DECLARE_PY_CHECKER(PyTuple)
DECLARE_PY_CHECKER(PyDict)
DECLARE_PY_CHECKER(PyTypeObject)
DECLARE_PY_CHECKER(PyFloat)
DECLARE_PY_CHECKER(PyBoolean)
DECLARE_PY_CHECKER(PyCodeObject)
DECLARE_PY_CHECKER(PyListIterator)
DECLARE_PY_CHECKER(PyTupleIterator)
DECLARE_PY_CHECKER(PyDictKeys)
DECLARE_PY_CHECKER(PyDictValues)
DECLARE_PY_CHECKER(PyDictItems)
DECLARE_PY_CHECKER(PyDictKeyIterator)
DECLARE_PY_CHECKER(PyDictItemIterator)
DECLARE_PY_CHECKER(PyDictValueIterator)
DECLARE_PY_CHECKER(FixedArray)
DECLARE_PY_CHECKER(MethodObject)
DECLARE_PY_CHECKER(Cell)
DECLARE_PY_CHECKER(HeapObject)
DECLARE_PY_CHECKER(GcAbleObject)
#undef DECLARE_PY_CHECKER

#define DECLARE_PY_CHECKER_WITH_ISOLATE(name)               \
  bool Is##name(Tagged<PyObject> object, Isolate* isolate); \
  bool Is##name(Handle<PyObject> object, Isolate* isolate);

DECLARE_PY_CHECKER_WITH_ISOLATE(PyNone)
DECLARE_PY_CHECKER_WITH_ISOLATE(PyModule)
DECLARE_PY_CHECKER_WITH_ISOLATE(PyFunction)
DECLARE_PY_CHECKER_WITH_ISOLATE(NormalPyFunction)
DECLARE_PY_CHECKER_WITH_ISOLATE(NativePyFunction)
DECLARE_PY_CHECKER_WITH_ISOLATE(PyTrue)
DECLARE_PY_CHECKER_WITH_ISOLATE(PyFalse)
#undef DECLARE_PY_CHECKER_WITH_ISOLATE

// 支持被用户Python代码显式继承的内建类型 Like 与 Exact 语义约定：
// - IsXxx：Like
//   - 如属于具有特殊内存布局的内建容器，采用 native layout kind 进行判定
//   - 否则采用 klass 全等判定作为 fallback
// - IsXxxExact：Exact（按 klass 全等判定）
#define DECLARE_PY_EXACT_CHECKER_WITH_ISOLATE(name)                \
  bool Is##name##Exact(Tagged<PyObject> object, Isolate* isolate); \
  bool Is##name##Exact(Handle<PyObject> object, Isolate* isolate);

PY_INHERITABLE_TYPE_IN_HEAP_LIST(DECLARE_PY_EXACT_CHECKER_WITH_ISOLATE)
#undef DECLARE_PY_EXACT_CHECKER_WITH_ISOLATE

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
