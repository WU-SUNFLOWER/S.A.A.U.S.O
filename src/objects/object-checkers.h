// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
#define SAAUSO_OBJECTS_OBJECT_CHECKERS_H_

#include "src/handles/handles.h"

namespace saauso::internal {

#define PY_TYPE_IN_HEAP_LIST(V) \
  V(PyTypeObject)               \
  V(PyString)                   \
  V(PyFunction)                 \
  V(PyFloat)                    \
  V(PyBoolean)                  \
  V(PyNone)                     \
  V(PyCodeObject)               \
  V(PyModule)                   \
  V(PyList)                     \
  V(PyListIterator)             \
  V(PyTuple)                    \
  V(PyTupleIterator)            \
  V(PyDict)                     \
  V(PyDictKeys)                 \
  V(PyDictValues)               \
  V(PyDictItems)                \
  V(PyDictKeyIterator)          \
  V(PyDictItemIterator)         \
  V(PyDictValueIterator)        \
  V(FixedArray)                 \
  V(MethodObject)               \
  V(Cell)

#define PY_TYPE_LIST(V) \
  V(PySmi)              \
  PY_TYPE_IN_HEAP_LIST(V)

#define DECLARE_PY_TYPE(name) class name;
PY_TYPE_LIST(DECLARE_PY_TYPE)
#undef DECLARE_PY_TYPE

class PyObject;

#define DECLARE_PY_CHECKER(name)          \
  bool Is##name(Tagged<PyObject> object); \
  bool Is##name(Handle<PyObject> object);

PY_TYPE_LIST(DECLARE_PY_CHECKER)
DECLARE_PY_CHECKER(NormalPyFunction)
DECLARE_PY_CHECKER(NativePyFunction)
DECLARE_PY_CHECKER(PyTrue)
DECLARE_PY_CHECKER(PyFalse)
DECLARE_PY_CHECKER(PyNativeFunction)
DECLARE_PY_CHECKER(HeapObject)
DECLARE_PY_CHECKER(GcAbleObject)

#undef DECLARE_PY_CHECKER

// 容器/字符串的 Like 与 Exact 语义约定：
// - IsPyString/IsPyList/IsPyDict/IsPyTuple：Like（按 native layout 判定）
// - IsPyStringExact/IsPyListExact/IsPyDictExact/IsPyTupleExact：Exact（按 klass 全等判定）
bool IsPyStringExact(Tagged<PyObject> object);
bool IsPyStringExact(Handle<PyObject> object);
bool IsPyListExact(Tagged<PyObject> object);
bool IsPyListExact(Handle<PyObject> object);
bool IsPyDictExact(Tagged<PyObject> object);
bool IsPyDictExact(Handle<PyObject> object);
bool IsPyTupleExact(Tagged<PyObject> object);
bool IsPyTupleExact(Handle<PyObject> object);

}  // namespace saauso::internal

#endif  // SAAUSO_OBJECTS_OBJECT_CHECKERS_H_
