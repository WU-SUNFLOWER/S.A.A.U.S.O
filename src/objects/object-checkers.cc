// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/objects/object-checkers.h"

#include "src/execution/isolate.h"
#include "src/objects/cell-klass.h"
#include "src/objects/fixed-array-klass.h"
#include "src/objects/klass.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict-views-klass.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-list-iterator-klass.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-module-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-tuple-iterator-klass.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-type-object-klass.h"

namespace saauso::internal {

namespace {

bool IsNativeLayoutKind(Tagged<PyObject> object,
                        NativeLayoutKind expected_kind) {
  return IsHeapObject(object) &&
         PyObject::GetHeapKlassUnchecked(object)->native_layout_kind() ==
             expected_kind;
}

}  // namespace

/////////////////////////////////////////////////////////////////////////
// 普通的对象类型 checker：
// - 对容器型 builtin，IsXxx 已经表示 like，走 native layout kind
// - 对其余非继承型 heap builtin，IsXxx 当前仍表示 exact

#define IMPL_PY_CHECKER_BY_KIND(Name, Kind)                       \
  bool Is##Name(Tagged<PyObject> object) {                        \
    return IsNativeLayoutKind(object, NativeLayoutKind::k##Kind); \
  }
IMPL_PY_CHECKER_BY_KIND(PyString, String)
IMPL_PY_CHECKER_BY_KIND(PyList, List)
IMPL_PY_CHECKER_BY_KIND(PyTuple, Tuple)
IMPL_PY_CHECKER_BY_KIND(PyDict, Dict)
IMPL_PY_CHECKER_BY_KIND(PyTypeObject, TypeObject)
IMPL_PY_CHECKER_BY_KIND(PyFloat, Float)
IMPL_PY_CHECKER_BY_KIND(PyCodeObject, CodeObject)
IMPL_PY_CHECKER_BY_KIND(PyListIterator, ListIterator)
IMPL_PY_CHECKER_BY_KIND(PyTupleIterator, TupleIterator)
IMPL_PY_CHECKER_BY_KIND(PyDictKeys, DictKeys)
IMPL_PY_CHECKER_BY_KIND(PyDictValues, DictValues)
IMPL_PY_CHECKER_BY_KIND(PyDictItems, DictItems)
IMPL_PY_CHECKER_BY_KIND(PyDictKeyIterator, DictKeyIterator)
IMPL_PY_CHECKER_BY_KIND(PyDictItemIterator, DictItemIterator)
IMPL_PY_CHECKER_BY_KIND(PyDictValueIterator, DictValueIterator)
IMPL_PY_CHECKER_BY_KIND(FixedArray, FixedArray)
IMPL_PY_CHECKER_BY_KIND(MethodObject, MethodObject)
IMPL_PY_CHECKER_BY_KIND(Cell, Cell)
IMPL_PY_CHECKER_BY_KIND(PyBoolean, Boolean)
#undef IMPL_PY_CHECKER_BY_KIND

#define IMPL_PY_CHECKER_BY_KLASS(Name)                                        \
  bool Is##Name(Tagged<PyObject> object, Isolate* isolate) {                  \
    return IsHeapObject(object) && PyObject::GetHeapKlassUnchecked(object) == \
                                       Name##Klass::GetInstance(isolate);     \
  }                                                                           \
  bool Is##Name(Handle<PyObject> object, Isolate* isolate) {                  \
    return Is##Name(*object, isolate);                                        \
  }
IMPL_PY_CHECKER_BY_KLASS(PyNone)
IMPL_PY_CHECKER_BY_KLASS(PyModule)
#undef IMPL_PY_CHECKER_BY_KLASS

/////////////////////////////////////////////////////////////////////////
// 其他特化 checker API

bool IsPyFunction(Tagged<PyObject> object, Isolate* isolate) {
  if (!IsHeapObject(object)) {
    return false;
  }
  Tagged<Klass> klass = PyObject::GetHeapKlassUnchecked(object);
  return klass == PyFunctionKlass::GetInstance(isolate) ||
         klass == NativeFunctionKlass::GetInstance(isolate);
}

bool IsPyFunction(Handle<PyObject> object, Isolate* isolate) {
  return IsPyFunction(*object, isolate);
}

bool IsNormalPyFunction(Tagged<PyObject> object, Isolate* isolate) {
  if (!IsHeapObject(object)) {
    return false;
  }
  return PyObject::GetHeapKlassUnchecked(object) ==
         PyFunctionKlass::GetInstance(isolate);
}

bool IsNormalPyFunction(Handle<PyObject> object, Isolate* isolate) {
  return IsNormalPyFunction(*object, isolate);
}

bool IsNativePyFunction(Tagged<PyObject> object, Isolate* isolate) {
  if (!IsHeapObject(object)) {
    return false;
  }
  return PyObject::GetHeapKlassUnchecked(object) ==
         NativeFunctionKlass::GetInstance(isolate);
}

bool IsNativePyFunction(Handle<PyObject> object, Isolate* isolate) {
  return IsNativePyFunction(*object, isolate);
}

bool IsPySmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

bool IsPyTrue(Tagged<PyObject> object, Isolate* isolate) {
  return object == Tagged<PyObject>(isolate->py_true_object());
}

bool IsPyTrue(Handle<PyObject> object, Isolate* isolate) {
  return IsPyTrue(*object, isolate);
}

bool IsPyFalse(Tagged<PyObject> object, Isolate* isolate) {
  return object == Tagged<PyObject>(isolate->py_false_object());
}

bool IsPyFalse(Handle<PyObject> object, Isolate* isolate) {
  return IsPyFalse(*object, isolate);
}

bool IsHeapObject(Tagged<PyObject> object) {
  return !object.is_null() && !IsPySmi(object);
}

bool IsGcAbleObject(Tagged<PyObject> object) {
  if (IsPySmi(object)) {
    return false;
  }
  if (PyObject::GetMarkWord(object).IsForwardingAddress()) {
    return true;
  }
  return !IsPyBoolean(object) && !IsPyNone(object, Isolate::Current());
}

/////////////////////////////////////////////////////////////////////////
// 支持接收 Handle 的 checker API

#define IMPL_PY_CHECKER_WITH_HANDLE_ARG(name) \
  bool Is##name(Handle<PyObject> object) {    \
    return Is##name(*object);                 \
  }

IMPL_PY_CHECKER_WITH_HANDLE_ARG(PySmi)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyString)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyList)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTuple)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDict)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTypeObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFloat)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyBoolean)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyCodeObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyListIterator)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTupleIterator)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictKeys)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictValues)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictItems)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictKeyIterator)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictItemIterator)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyDictValueIterator)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(FixedArray)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(MethodObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(Cell)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(HeapObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(GcAbleObject)
#undef IMPL_PY_CHECKER_WITH_HANDLE_ARG

/////////////////////////////////////////////////////////////////////////
// 针对支持被用户Python代码继承的类型，提供显式的 exact 语义 API。
// 基于 Klass 头指针进行精确判断。

// TODO: VM内部IsXxxx系列API，要求显式传入Isolate
#define IMPL_PY_EXACT_CHECKER_BY_KLASS(name)                                  \
  bool Is##name##Exact(Tagged<PyObject> object, Isolate* isolate) {           \
    return IsHeapObject(object) && PyObject::GetHeapKlassUnchecked(object) == \
                                       name##Klass::GetInstance(isolate);     \
  }                                                                           \
  bool Is##name##Exact(Handle<PyObject> object, Isolate* isolate) {           \
    return Is##name##Exact(*object, isolate);                                 \
  }

PY_INHERITABLE_TYPE_IN_HEAP_LIST(IMPL_PY_EXACT_CHECKER_BY_KLASS)
#undef IMPL_PY_EXACT_CHECKER_BY_KLASS

}  // namespace saauso::internal
