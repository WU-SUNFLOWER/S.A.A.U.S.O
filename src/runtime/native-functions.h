// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_
#define SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_

#include "src/objects/py-object.h"

namespace saauso::internal {

Handle<PyObject> Native_Print(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs);

Handle<PyObject> Native_Len(Handle<PyObject> host,
                            Handle<PyTuple> args,
                            Handle<PyDict> kwargs);

Handle<PyObject> Native_IsInstance(Handle<PyObject> host,
                                   Handle<PyTuple> args,
                                   Handle<PyDict> kwargs);

Handle<PyObject> Native_BuildTypeObject(Handle<PyObject> host,
                                        Handle<PyTuple> args,
                                        Handle<PyDict> kwargs);

Handle<PyObject> Native_Sysgc(Handle<PyObject> host,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_NATIVE_FUNCTIONS_H_
