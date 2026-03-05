// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_OBJECTS_OBJECT_TYPE_LIST_H_
#define SAAUSO_OBJECTS_OBJECT_TYPE_LIST_H_

// S.A.A.U.S.O 中支持被用户层定义的Python类继承的
// 堆上内建类型。
// 每种类型都有可被继承的内存布局约定，
// 请参考 src/objects/klass.h 当中的 NativeLayoutKind 枚举体。
#define PY_INHERITABLE_TYPE_IN_HEAP_LIST(V) \
  V(PyString)                               \
  V(PyList)                                 \
  V(PyTuple)                                \
  V(PyDict)

#define PY_NOT_INHERITABLE_TYPE_IN_HEAP_LIST(V) \
  V(PyTypeObject)                               \
  V(PyFunction)                                 \
  V(PyFloat)                                    \
  V(PyBoolean)                                  \
  V(PyNone)                                     \
  V(PyCodeObject)                               \
  V(PyModule)                                   \
  V(PyListIterator)                             \
  V(PyTupleIterator)                            \
  V(PyDictKeys)                                 \
  V(PyDictValues)                               \
  V(PyDictItems)                                \
  V(PyDictKeyIterator)                          \
  V(PyDictItemIterator)                         \
  V(PyDictValueIterator)                        \
  V(FixedArray)                                 \
  V(MethodObject)                               \
  V(Cell)

// S.A.A.U.S.O 中所有在堆区被分配的内建类型
#define PY_TYPE_IN_HEAP_LIST(V)       \
  PY_INHERITABLE_TYPE_IN_HEAP_LIST(V) \
  PY_NOT_INHERITABLE_TYPE_IN_HEAP_LIST(V)

#define PY_TYPE_LIST(V) \
  V(PySmi)              \
  PY_TYPE_IN_HEAP_LIST(V)

#endif  // SAAUSO_OBJECTS_OBJECT_TYPE_LIST_H_
