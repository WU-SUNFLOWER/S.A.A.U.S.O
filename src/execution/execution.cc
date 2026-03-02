// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/execution.h"

#include "src/execution/isolate.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-object.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"

namespace saauso::internal {

MaybeHandle<PyObject> Execution::Call(Isolate* isolate,
                                      Handle<PyObject> callable,
                                      Handle<PyObject> host,
                                      Handle<PyTuple> pos_args,
                                      Handle<PyDict> kw_args) {
  return isolate->interpreter()->CallPython(callable, host, pos_args, kw_args);
}

MaybeHandle<PyObject> Execution::Call(Isolate* isolate,
                                      Handle<PyObject> callable,
                                      Handle<PyObject> host,
                                      Handle<PyTuple> pos_args,
                                      Handle<PyDict> kw_args,
                                      Handle<PyDict> bound_locals) {
  return isolate->interpreter()->CallPython(callable, host, pos_args, kw_args,
                                            bound_locals);
}

Handle<PyDict> Execution::CurrentFrameGlobals(Isolate* isolate) {
  return isolate->interpreter()->CurrentFrameGlobals();
}

Handle<PyDict> Execution::CurrentFrameLocals(Isolate* isolate) {
  return isolate->interpreter()->CurrentFrameLocals();
}

}  // namespace saauso::internal
