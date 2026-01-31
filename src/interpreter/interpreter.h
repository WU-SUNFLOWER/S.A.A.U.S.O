// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_INTERPRETER_H_
#define SAAUSO_RUNTIME_INTERPRETER_H_

#include "src/handles/handles.h"

namespace saauso::internal {

class Isolate;
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
  explicit Interpreter(Isolate* isolate);

  void Run(Handle<PyCodeObject> code_object);

  Handle<PyObject> CallPython(Handle<PyObject> callable,
                              Handle<PyTuple> args,
                              Handle<PyDict> kwargs);

  Handle<PyDict> builtins() const;
  Handle<PyTuple> kwarg_keys() const;

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  void InvokeCallable(Handle<PyObject> callable,
                      Handle<PyObject> host,
                      Handle<PyTuple> actual_args,
                      Handle<PyTuple> kwarg_keys);

  void InvokeCallableWithNormalizedArgs(Handle<PyObject> callable,
                                        Handle<PyObject> host,
                                        Handle<PyTuple> pos_args,
                                        Handle<PyDict> kw_args);

  void NormalizeCallable(Handle<PyObject>& callable, Handle<PyObject>& host);
  void NormalizeArguments(Handle<PyTuple> actual_args,
                          Handle<PyTuple> kwarg_keys,
                          Handle<PyTuple>& pos_args,
                          Handle<PyDict>& kw_args);

  void EnterFrame(FrameObject* frame);
  void EvalCurrentFrame();
  void LeaveCurrentFrame();
  void DestroyCurrentFrame();

  void PopCallTarget(Handle<PyObject>& callable, Handle<PyObject>& host);

  Handle<PyObject> ReleaseReturnValue();
  Handle<PyTuple> ReleaseKwArgKeys();

  Isolate* isolate_;

  // PyDict* builtins_;
  Tagged<PyObject> builtins_{kNullAddress};

  // 栈帧上下文切换前，临时储存函数返回值结果
  Tagged<PyObject> ret_value_{kNullAddress};

  // 临时储存发起函数调用时传入的实际键值对参数的所有键
  Tagged<PyObject> kwarg_keys_{kNullAddress};

  FrameObject* current_frame_{nullptr};

  void* dispatch_table_[256];
  bool dispatch_table_initialized_{false};
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_INTERPRETER_H_
