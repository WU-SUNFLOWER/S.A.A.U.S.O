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
         PyObject::GetKlass(object)->native_layout_kind() == expected_kind;
}

}  // namespace

#define IMPL_PY_CHECKER_WITH_HANDLE_ARG(name) \
  bool Is##name(Handle<PyObject> object) {    \
    return Is##name(*object);                 \
  }

#define IMPL_PY_CHECKER_BY_KLASS(name)         \
  bool Is##name(Tagged<PyObject> object) {     \
    return PyObject::GetKlass(object).ptr() == \
           name##Klass::GetInstance().ptr();   \
  }                                            \
  IMPL_PY_CHECKER_WITH_HANDLE_ARG(name)

#define IMPL_PY_EXACT_CHECKER_BY_KLASS(name)      \
  bool Is##name##Exact(Tagged<PyObject> object) { \
    return PyObject::GetKlass(object).ptr() ==    \
           name##Klass::GetInstance().ptr();      \
  }                                               \
  bool Is##name##Exact(Handle<PyObject> object) { \
    return Is##name##Exact(*object);              \
  }

IMPL_PY_CHECKER_BY_KLASS(PyTypeObject)
IMPL_PY_CHECKER_BY_KLASS(PyFloat)
IMPL_PY_CHECKER_BY_KLASS(PyBoolean)
IMPL_PY_CHECKER_BY_KLASS(PyNone)
IMPL_PY_CHECKER_BY_KLASS(PyCodeObject)
IMPL_PY_CHECKER_BY_KLASS(PyModule)
IMPL_PY_CHECKER_BY_KLASS(PyListIterator)
IMPL_PY_CHECKER_BY_KLASS(PyTupleIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDictKeys)
IMPL_PY_CHECKER_BY_KLASS(PyDictValues)
IMPL_PY_CHECKER_BY_KLASS(PyDictItems)
IMPL_PY_CHECKER_BY_KLASS(PyDictKeyIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDictItemIterator)
IMPL_PY_CHECKER_BY_KLASS(PyDictValueIterator)
IMPL_PY_CHECKER_BY_KLASS(FixedArray)
IMPL_PY_CHECKER_BY_KLASS(MethodObject)
IMPL_PY_CHECKER_BY_KLASS(Cell)

bool IsPyString(Tagged<PyObject> object) {
  return IsNativeLayoutKind(object, NativeLayoutKind::kString);
}
bool IsPyString(Handle<PyObject> object) {
  return IsPyString(*object);
}
bool IsPyList(Tagged<PyObject> object) {
  return IsNativeLayoutKind(object, NativeLayoutKind::kList);
}
bool IsPyList(Handle<PyObject> object) {
  return IsPyList(*object);
}
bool IsPyTuple(Tagged<PyObject> object) {
  return IsNativeLayoutKind(object, NativeLayoutKind::kTuple);
}
bool IsPyTuple(Handle<PyObject> object) {
  return IsPyTuple(*object);
}
bool IsPyDict(Tagged<PyObject> object) {
  return IsNativeLayoutKind(object, NativeLayoutKind::kDict);
}
bool IsPyDict(Handle<PyObject> object) {
  return IsPyDict(*object);
}

IMPL_PY_EXACT_CHECKER_BY_KLASS(PyString)
IMPL_PY_EXACT_CHECKER_BY_KLASS(PyList)
IMPL_PY_EXACT_CHECKER_BY_KLASS(PyTuple)
IMPL_PY_EXACT_CHECKER_BY_KLASS(PyDict)

#undef IMPL_PY_CHECKER_BY_KLASS
#undef IMPL_PY_EXACT_CHECKER_BY_KLASS

bool IsPyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == PyFunctionKlass::GetInstance() ||
         PyObject::GetKlass(object) == NativeFunctionKlass::GetInstance();
}

bool IsNormalPyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == PyFunctionKlass::GetInstance();
}

bool IsNativePyFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object) == NativeFunctionKlass::GetInstance();
}

bool IsPySmi(Tagged<PyObject> object) {
  return object.IsSmi();
}

bool IsPyTrue(Tagged<PyObject> object) {
  return object.ptr() ==
         Tagged<PyObject>(Isolate::Current()->py_true_object()).ptr();
}

bool IsPyFalse(Tagged<PyObject> object) {
  return object.ptr() ==
         Tagged<PyObject>(Isolate::Current()->py_false_object()).ptr();
}

bool IsHeapObject(Tagged<PyObject> object) {
  return !object.is_null() && !IsPySmi(object);
}

bool IsPyNativeFunction(Tagged<PyObject> object) {
  return PyObject::GetKlass(object).ptr() ==
         NativeFunctionKlass::GetInstance().ptr();
}

bool IsGcAbleObject(Tagged<PyObject> object) {
  if (IsPySmi(object)) {
    return false;
  }
  if (PyObject::GetMarkWord(object).IsForwardingAddress()) {
    return true;
  }
  return !IsPyBoolean(object) && !IsPyNone(object);
}

IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(NormalPyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(NativePyFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PySmi)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyTrue)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyFalse)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(HeapObject)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(PyNativeFunction)
IMPL_PY_CHECKER_WITH_HANDLE_ARG(GcAbleObject)
#undef IMPL_PY_CHECKER_WITH_HANDLE_ARG

}  // namespace saauso::internal
