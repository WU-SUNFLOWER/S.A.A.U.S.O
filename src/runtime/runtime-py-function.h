// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_RUNTIME_PY_FUNCTION_H_
#define SAAUSO_RUNTIME_RUNTIME_PY_FUNCTION_H_

#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"

namespace saauso::internal {

class PyString;
class PyFunction;
class MethodObject;
class Isolate;
class PyObject;
class PyTuple;
class PyDict;

MaybeHandle<PyString> Runtime_NewFunctionRepr(Handle<PyFunction> func);

MaybeHandle<PyString> Runtime_NewMethodObjectRepr(Handle<MethodObject> method);

Maybe<void> Runtime_NormalizeNativeMethodCall(Isolate* isolate,
                                              Handle<PyFunction> func,
                                              Handle<PyObject>& receiver,
                                              Handle<PyTuple>& args);

MaybeHandle<PyObject> Runtime_CallNativePyFunction(Isolate* isolate,
                                                   Handle<PyFunction> func,
                                                   Handle<PyObject> receiver,
                                                   Handle<PyTuple> args,
                                                   Handle<PyDict> kwargs);

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_RUNTIME_PY_FUNCTION_H_
