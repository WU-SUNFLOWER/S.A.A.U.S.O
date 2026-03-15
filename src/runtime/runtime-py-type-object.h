// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_TYPE_OBJECT_H_
#define SAAUSO_RUNTIME_RUNTIME_TYPE_OBJECT_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyString;
class PyTypeObject;

MaybeHandle<PyObject> Runtime_NewType(Isolate* isolate,
                                      Handle<PyObject> args,
                                      Handle<PyObject> kwargs);

MaybeHandle<PyString> Runtime_NewTypeObjectRepr(
    Handle<PyTypeObject> type_object);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_TYPE_OBJECT_H_
