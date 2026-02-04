// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"

#ifndef SAAUSO_RUNTIME_RUNTIME_H_
#define SAAUSO_RUNTIME_RUNTIME_H_

namespace saauso::internal {

class Klass;
class PyObject;
class PyList;
class PyDict;
class PyString;
class PyTypeObject;
class PyTuple;

bool Runtime_PyObjectIsTrue(Handle<PyObject> object);

bool Runtime_PyObjectIsTrue(Tagged<PyObject> object);

void Runtime_ExtendListByItratableObject(Handle<PyList> list,
                                         Handle<PyObject> iteratable);

Handle<PyTuple> Runtime_UnpackIterableObjectToTuple(Handle<PyObject> iterable);

Handle<PyTuple> Runtime_IntrinsicListToTuple(Handle<PyObject> list);

int64_t Runtime_DecodeIntLikeOrDie(Tagged<PyObject> value);

bool Runtime_IsPyObjectCallable(Tagged<PyObject> object);

Handle<PyTypeObject> Runtime_CreatePythonClass(Handle<PyString> class_name,
                                               Handle<PyDict> class_properties,
                                               Handle<PyList> supers);

Handle<PyObject> Runtime_FindPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name);

Handle<PyObject> Runtime_FindPropertyInKlassMro(Tagged<Klass> klass,
                                                Handle<PyObject> prop_name);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_H_
