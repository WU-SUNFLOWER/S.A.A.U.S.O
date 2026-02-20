// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SRC_EXECUTION_EXECUTION_H_
#define SRC_EXECUTION_EXECUTION_H_

#include "src/common/globals.h"
#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
class PyObject;
class PyTuple;
class PyDict;
class PyTypeObject;

// execution 层的全静态门面：收敛“执行 Python 代码/调用可调用对象”的入口。
// 该门面本身不维护任何状态，状态归属由 Isolate/ExceptionState 等类型承载。
class Execution final : public AllStatic {
 public:
  static Handle<PyObject> Call(Isolate* isolate,
                               Handle<PyObject> callable,
                               Handle<PyObject> host,
                               Handle<PyTuple> pos_args,
                               Handle<PyDict> kw_args);

  static Handle<PyObject> Call(Isolate* isolate,
                               Handle<PyObject> callable,
                               Handle<PyObject> host,
                               Handle<PyTuple> pos_args,
                               Handle<PyDict> kw_args,
                               Handle<PyDict> bound_locals);

  static Handle<PyDict> builtins(Isolate* isolate);

  static Handle<PyDict> CurrentFrameGlobals(Isolate* isolate);
  static Handle<PyDict> CurrentFrameLocals(Isolate* isolate);
};

}  // namespace saauso::internal

#endif  // SRC_EXECUTION_EXECUTION_H_
