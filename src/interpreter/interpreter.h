// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#ifndef SAAUSO_RUNTIME_INTERPRETER_H_
#define SAAUSO_RUNTIME_INTERPRETER_H_

#include "src/execution/exception-state.h"
#include "src/handles/handles.h"
#include "src/handles/maybe-handles.h"
#include "src/utils/vector.h"

namespace saauso::internal {

class Isolate;
class PyObject;
class PyCodeObject;
class FrameObject;
class PyFunction;
class PyList;
class PyDict;
class PyTuple;
class PyString;
class ObjectVisitor;

class Interpreter {
 public:
  explicit Interpreter(Isolate* isolate);

  Maybe<void> Run(Handle<PyFunction> boilerplate);

  MaybeHandle<PyObject> CallPython(Handle<PyObject> callable,
                                   Handle<PyObject> host,
                                   Handle<PyTuple> pos_args,
                                   Handle<PyDict> kw_args);

  MaybeHandle<PyObject> CallPython(Handle<PyObject> callable,
                                   Handle<PyObject> host,
                                   Handle<PyTuple> pos_args,
                                   Handle<PyDict> kw_args,
                                   Handle<PyDict> bound_locals);

  Handle<PyDict> CurrentFrameGlobals() const;
  Handle<PyDict> CurrentFrameLocals() const;

  Handle<PyTuple> kwarg_keys() const;

  // 返回当前处于“已捕获态”的异常对象（用于 except 语义与 sys.exc_info 族
  // API）。
  Tagged<PyObject> caught_exception_tagged() const;
  Handle<PyObject> caught_exception() const;

  Tagged<PyObject> pending_exception_tagged() const;
  Handle<PyObject> pending_exception() const;
  void ClearPendingException();

  // GC接口
  void Iterate(ObjectVisitor* v);

 private:
  Maybe<void> InvokeCallable(Handle<PyObject> callable,
                             Handle<PyObject> host,
                             Handle<PyTuple> actual_args,
                             Handle<PyTuple> kwarg_keys);

  Maybe<void> InvokeCallableWithNormalizedArgs(Handle<PyObject> callable,
                                               Handle<PyObject> host,
                                               Handle<PyTuple> pos_args,
                                               Handle<PyDict> kw_args);

  template <typename... ExtendArgs>
  MaybeHandle<PyObject> CallPythonImpl(Handle<PyObject> callable,
                                       Handle<PyObject> host,
                                       Handle<PyTuple> args,
                                       Handle<PyDict> kwargs,
                                       ExtendArgs... extend_args);

  void NormalizeCallable(Handle<PyObject>& callable, Handle<PyObject>& host);
  void NormalizeArguments(Handle<PyTuple> actual_args,
                          Handle<PyTuple> kwarg_keys,
                          Handle<PyTuple>& pos_args,
                          Handle<PyDict>& kw_args);

  void EnterFrame(FrameObject* frame);
  void EvalCurrentFrame();
  void LeaveCurrentFrame();
  void DestroyCurrentFrame();
  void UnwindCurrentFrameForException();

  void PopCallTarget(Handle<PyObject>& callable, Handle<PyObject>& host);

  Handle<PyObject> ReleaseReturnValue();
  Handle<PyTuple> ReleaseKwArgKeys();

  Isolate* isolate_;

  // 栈帧上下文切换前，临时储存函数返回值结果
  Tagged<PyObject> ret_value_{kNullAddress};

  // 临时储存发起函数调用时传入的实际键值对参数的所有键
  Tagged<PyObject> kwarg_keys_{kNullAddress};

  // 当进入某个 handler 时，把其 lasti
  // 语义临时缓存下来，供随后 PUSH_EXC_INFO 写入 caught_exception_origin_ip_。
  int stack_exception_origin_pc_{ExceptionState::kInvalidProgramCounter};

  // 当前处于“已捕获态”的异常。即当前 except 块正在处理/持有的异常。
  // 用于 PUSH_EXC_INFO/POP_EXCEPT 恢复。
  Tagged<PyObject> caught_exception_{kNullAddress};
  // caught_exception_对应的 origin ip，供
  // `raise`(无参)再抛时恢复 lasti 语义。
  int caught_exception_origin_pc_{ExceptionState::kInvalidProgramCounter};
  Vector<int> caught_exception_origin_pc_stack_;

  FrameObject* current_frame_{nullptr};

  void* dispatch_table_[256];
  bool dispatch_table_initialized_{false};
};

}  // namespace saauso::internal

#endif  // SAAUSO_RUNTIME_INTERPRETER_H_
