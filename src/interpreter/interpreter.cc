// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/interpreter.h"

#include "src/builtins/builtins-definitions.h"
#include "src/execution/exception-utils.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/frame-object-builder.h"
#include "src/interpreter/frame-object.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-float-klass.h"
#include "src/objects/py-function-klass.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object-klass.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs-klass.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi-klass.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string-klass.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple-klass.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object-klass.h"
#include "src/objects/py-type-object.h"
#include "src/objects/visitors.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Interpreter::Interpreter(Isolate* isolate) : isolate_(isolate) {}

Handle<PyTuple> Interpreter::kwarg_keys() const {
  return handle(Tagged<PyTuple>::cast(kwarg_keys_));
}

Tagged<PyObject> Interpreter::caught_exception_tagged() const {
  return Tagged<PyObject>::cast(caught_exception_);
}

Handle<PyObject> Interpreter::caught_exception() const {
  return handle(caught_exception_tagged());
}

Tagged<PyObject> Interpreter::pending_exception_tagged() const {
  return isolate_->exception_state()->pending_exception_tagged();
}

Handle<PyObject> Interpreter::pending_exception() const {
  return isolate_->exception_state()->pending_exception();
}

void Interpreter::ClearPendingException() {
  isolate_->exception_state()->Clear();
}

Handle<PyDict> Interpreter::CurrentFrameGlobals() const {
  assert(current_frame_ != nullptr);
  return current_frame_->globals();
}

Handle<PyDict> Interpreter::CurrentFrameLocals() const {
  assert(current_frame_ != nullptr);
  return current_frame_->locals();
}

Maybe<void> Interpreter::Run(Handle<PyFunction> boilerplate) {
  Handle<PyDict> globals = PyDict::NewInstance();
  RETURN_ON_EXCEPTION(isolate_, PyDict::Put(globals, ST(name), ST(main)));

  boilerplate->set_func_globals(globals);

  FrameObject* frame;
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, frame,
      FrameObjectBuilder::BuildFastPath(boilerplate, Handle<PyObject>::null(),
                                        Handle<PyTuple>::null(),
                                        Handle<PyTuple>::null(), globals));

  EnterFrame(frame);
  EvalCurrentFrame();
  DestroyCurrentFrame();

  // 如果执行Python代码的过程中出现了未被处理的异常，
  // 则显式向上传播。
  if (isolate_->HasPendingException()) {
    return kNullMaybe;
  }

  return JustVoid();
}

MaybeHandle<PyObject> Interpreter::CallPython(Handle<PyObject> callable,
                                              Handle<PyObject> host,
                                              Handle<PyTuple> pos_args,
                                              Handle<PyDict> kw_args) {
  return CallPythonImpl(callable, host, pos_args, kw_args);
}

MaybeHandle<PyObject> Interpreter::CallPython(Handle<PyObject> callable,
                                              Handle<PyObject> host,
                                              Handle<PyTuple> pos_args,
                                              Handle<PyDict> kw_args,
                                              Handle<PyDict> bound_locals) {
  return CallPythonImpl(callable, host, pos_args, kw_args, bound_locals);
}

// private
template <typename... ExtendArgs>
MaybeHandle<PyObject> Interpreter::CallPythonImpl(Handle<PyObject> callable,
                                                  Handle<PyObject> host,
                                                  Handle<PyTuple> pos_args,
                                                  Handle<PyDict> kw_args,
                                                  ExtendArgs... extend_args) {
  EscapableHandleScope scope;

  NormalizeCallable(callable, host);

  Handle<PyObject> result = Handle<PyObject>::null();

  // 如果是普通的python函数，那么请求解释器进行处理
  if (IsNormalPyFunction(callable)) {
    FrameObject* frame;
    // 构建栈帧
    ASSIGN_RETURN_ON_EXCEPTION(isolate_, frame,
                               FrameObjectBuilder::BuildSlowPath(
                                   Handle<PyFunction>::cast(callable), host,
                                   pos_args, kw_args, extend_args...));

    // 确保Python栈帧处理结束后能够退回到C++栈帧
    frame->set_is_entry_frame(true);

    // 调用解释器处理栈帧
    EnterFrame(frame);
    EvalCurrentFrame();

    // 手工提取返回值，然后弹出当前python栈帧
    result = ReleaseReturnValue();
    DestroyCurrentFrame();

    if (isolate_->HasPendingException()) {
      return kNullMaybeHandle;
    }

    return scope.Escape(result);
  }

  // Fast Path: 如果是NativeFunction，直接执行调用
  if (IsNativePyFunction(callable)) {
    auto func_object = Handle<PyFunction>::cast(callable);
    auto* native_func_ptr = func_object->native_func();
    ASSIGN_RETURN_ON_EXCEPTION(isolate_, result,
                               native_func_ptr(host, pos_args, kw_args));

    return scope.Escape(result);
  }

  // 兜底：尝试调用callable的call虚方法
  // 如果对象无法被调用，执行PyObject::Call后会抛出错误。
  // 类似于TypeError: 'xxx' object is not callable
  if (!PyObject::Call(callable, host, pos_args, kw_args).ToHandle(&result)) {
    return kNullMaybeHandle;
  }
  return scope.Escape(result);
}

// private
void Interpreter::NormalizeCallable(Handle<PyObject>& callable,
                                    Handle<PyObject>& host) {
  // 如果是对象方法，那么进行解包
  if (IsMethodObject(callable)) {
    // 如果传入的已经是待解包的host，理论上不应该再显式传入host
    assert(host.is_null());
    auto method = Handle<MethodObject>::cast(callable);
    callable = method->func();
    host = method->owner();
  }
}

// private
void Interpreter::NormalizeArguments(Handle<PyTuple> actual_args,
                                     Handle<PyTuple> kwarg_keys,
                                     Handle<PyTuple>& pos_args,
                                     Handle<PyDict>& kw_args) {
  // 如果有有效的键值对参数，从全体args当中单独提取出来
  if (!kwarg_keys.is_null()) {
    kw_args = PyDict::NewInstance();

    assert(!actual_args.is_null());
    auto actual_args_size = actual_args->length();

    for (auto i = 0; i < kwarg_keys->length(); ++i) {
      (void)PyDict::Put(kw_args, kwarg_keys->Get(kwarg_keys->length() - i - 1),
                        actual_args->Get(actual_args_size - i - 1));
    }

    // 从actual_args尾部截去提取出来的参数，剩余部分作为pos_args
    actual_args->ShrinkInternal(actual_args->length() - kwarg_keys->length());
    pos_args = actual_args;
  } else {
    pos_args = actual_args;
  }
}

// private
Handle<PyObject> Interpreter::ReleaseReturnValue() {
  auto result = handle(ret_value_);
  ret_value_ = Tagged<PyObject>::null();
  return result;
}

// private
Handle<PyTuple> Interpreter::ReleaseKwArgKeys() {
  auto result = Handle<PyTuple>::cast(handle(kwarg_keys_));
  kwarg_keys_ = Tagged<PyObject>::null();
  return result;
}

// public
void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&ret_value_);
  v->VisitPointer(&kwarg_keys_);
  v->VisitPointer(&caught_exception_);

  FrameObject* frame = current_frame_;
  // 遍历函数调用栈
  while (frame != nullptr) {
    frame->Iterate(v);
    frame = frame->caller();
  }
}

}  // namespace saauso::internal
