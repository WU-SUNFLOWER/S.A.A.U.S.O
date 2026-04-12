// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/execution.h"

#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-object.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

// static
MaybeHandle<PyObject> Execution::RunScriptAsMain(
    Isolate* isolate,
    Handle<PyFunction> boilerplate) {
  EscapableHandleScope scope(isolate);
  Handle<PyDict> globals = PyDict::New(isolate);
  RETURN_ON_EXCEPTION(isolate,
                      PyDict::Put(globals, ST(name, isolate), ST(main, isolate),
                                  isolate));

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result, CallScript(isolate, boilerplate, globals, globals));
  return scope.Escape(result);
}

// static
MaybeHandle<PyObject> Execution::CallScript(Isolate* isolate,
                                            Handle<PyFunction> boilerplate,
                                            Handle<PyDict> locals,
                                            Handle<PyDict> globals) {
  EscapableHandleScope scope(isolate);

  boilerplate->set_func_globals(globals);

  Handle<PyObject> result;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate, result,
      Call(isolate, boilerplate, Handle<PyObject>::null(),
           Handle<PyTuple>::null(), Handle<PyDict>::null(), locals));

  return scope.Escape(result);
}

// static
MaybeHandle<PyObject> Execution::Call(Isolate* isolate,
                                      Handle<PyObject> callable,
                                      Handle<PyObject> receiver,
                                      Handle<PyTuple> pos_args,
                                      Handle<PyDict> kw_args) {
  return isolate->interpreter()->CallPython(callable, receiver, pos_args,
                                            kw_args);
}

// static
MaybeHandle<PyObject> Execution::Call(Isolate* isolate,
                                      Handle<PyObject> callable,
                                      Handle<PyObject> receiver,
                                      Handle<PyTuple> pos_args,
                                      Handle<PyDict> kw_args,
                                      Handle<PyDict> bound_locals) {
  return isolate->interpreter()->CallPython(callable, receiver, pos_args,
                                            kw_args, bound_locals);
}

// static
Handle<PyDict> Execution::CurrentFrameGlobals(Isolate* isolate) {
  return isolate->interpreter()->CurrentFrameGlobals();
}

// static
Handle<PyDict> Execution::CurrentFrameLocals(Isolate* isolate) {
  return isolate->interpreter()->CurrentFrameLocals();
}

}  // namespace saauso::internal
