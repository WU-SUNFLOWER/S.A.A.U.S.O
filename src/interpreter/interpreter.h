// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_INTERPRETER_H_
#define SAAUSO_RUNTIME_INTERPRETER_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class PyObject;
class PyCodeObject;
class FrameObject;
class PyFunction;
class PyList;
class PyDict;
class PyTuple;
class ObjectVisitor;

class Interpreter {
 public:
  Interpreter();

  void Run(Handle<PyCodeObject> code_object);

  void InvokeCallable(Handle<PyObject> callable,
                      Handle<PyTuple> args,
                      Handle<PyDict> kwargs);
  void BuildFrame(Handle<PyFunction> func, Handle<PyTuple> args);
  void LeaveFrame();
  void DestroyFrame();

  Handle<PyObject> CallVirtual(Handle<PyObject> callable, Handle<PyTuple> args);

  Handle<PyDict> builtins() const;

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  // PyDict* builtins_;
  Tagged<PyObject> builtins_{kNullAddress};

  // 栈帧上下文切换前，临时储存函数返回值结果
  Tagged<PyObject> ret_value_{kNullAddress};

  FrameObject* frame_;

  void* dispatch_table_[256];
  bool dispatch_table_initialized_{false};
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_INTERPRETER_H_
