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
#include "src/runtime/runtime-py-function.h"

namespace saauso::internal {

Interpreter::Interpreter(Isolate* isolate) : isolate_(isolate) {}

Handle<PyTuple> Interpreter::kwarg_keys() const {
  return handle(Tagged<PyTuple>::cast(kwarg_keys_), isolate_);
}

Tagged<PyObject> Interpreter::caught_exception_tagged() const {
  return Tagged<PyObject>::cast(caught_exception_);
}

Handle<PyObject> Interpreter::caught_exception() const {
  return handle(caught_exception_tagged(), isolate_);
}

void Interpreter::ClearPendingException() {
  isolate_->exception_state()->Clear();
}

Handle<PyDict> Interpreter::CurrentFrameGlobals() const {
  assert(current_frame_ != nullptr);
  return current_frame_->globals(isolate_);
}

Handle<PyDict> Interpreter::CurrentFrameLocals() const {
  assert(current_frame_ != nullptr);
  return current_frame_->locals(isolate_);
}

MaybeHandle<PyObject> Interpreter::CallPython(Handle<PyObject> callable,
                                              Handle<PyObject> receiver,
                                              Handle<PyTuple> pos_args,
                                              Handle<PyDict> kw_args) {
  return CallPythonImpl(callable, receiver, pos_args, kw_args);
}

MaybeHandle<PyObject> Interpreter::CallPython(Handle<PyObject> callable,
                                              Handle<PyObject> receiver,
                                              Handle<PyTuple> pos_args,
                                              Handle<PyDict> kw_args,
                                              Handle<PyDict> bound_locals) {
  return CallPythonImpl(callable, receiver, pos_args, kw_args, bound_locals);
}

// private
template <typename... ExtendArgs>
MaybeHandle<PyObject> Interpreter::CallPythonImpl(Handle<PyObject> callable,
                                                  Handle<PyObject> receiver,
                                                  Handle<PyTuple> pos_args,
                                                  Handle<PyDict> kw_args,
                                                  ExtendArgs... extend_args) {
  EscapableHandleScope scope(isolate_);

  NormalizeCallable(callable, receiver);

  Handle<PyObject> result = Handle<PyObject>::null();

  // 如果是普通的python函数，那么请求解释器进行处理
  if (IsNormalPyFunction(callable, isolate_)) {
    FrameObject* frame;
    // 构建栈帧
    ASSIGN_RETURN_ON_EXCEPTION(
        isolate_, frame,
        FrameObjectBuilder::BuildSlowPath(
            isolate_, Handle<PyFunction>::cast(callable), receiver, pos_args,
            kw_args, extend_args...));

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

  // 处理其他类型的callable
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, result,
      CallNonNormalFunction(callable, receiver, pos_args, kw_args));

  return scope.Escape(result);
}

// private
MaybeHandle<PyObject> Interpreter::CallNonNormalFunction(
    Handle<PyObject> callable,
    Handle<PyObject> receiver,
    Handle<PyTuple> pos_args,
    Handle<PyDict> kw_args) {
  // 该API只允许传入非普通Python函数的callable对象
  assert(!IsNormalPyFunction(callable, isolate_));

  Handle<PyObject> result;

  // Fast Path: 如果是NativeFunction，直接执行调用
  if (IsNativePyFunction(callable, isolate_)) {
    ASSIGN_RETURN_ON_EXCEPTION(isolate_, result,
                               Runtime_CallNativePyFunction(
                                   isolate_, Handle<PyFunction>::cast(callable),
                                   receiver, pos_args, kw_args));

    return result;
  }

  // 兜底：尝试调用callable的call虚方法
  // 如果对象无法被调用，执行PyObject::Call后会抛出错误。
  // 类似于TypeError: 'xxx' object is not callable
  ASSIGN_RETURN_ON_EXCEPTION(
      isolate_, result,
      PyObject::Call(isolate_, callable, receiver, pos_args, kw_args));

  return result;
}

// private
void Interpreter::NormalizeCallable(Handle<PyObject>& callable,
                                    Handle<PyObject>& receiver) {
  // 如果是对象方法，那么进行解包
  if (IsMethodObject(callable)) {
    // 如果传入的已经是待解包的 receiver，理论上不应该再显式传入 receiver。
    assert(receiver.is_null());
    auto method = Handle<MethodObject>::cast(callable);
    callable = method->func(isolate_);
    receiver = method->owner(isolate_);
  }
}

// private
Maybe<void> Interpreter::NormalizeArguments(Handle<PyTuple> actual_args,
                                            Handle<PyTuple> kwarg_keys,
                                            Handle<PyTuple>& pos_args,
                                            Handle<PyDict>& kw_args) {
  // 如果有有效的键值对参数，从全体args当中单独提取出来
  if (!kwarg_keys.is_null()) {
    kw_args = PyDict::New(isolate_);

    assert(!actual_args.is_null());
    auto actual_args_size = actual_args->length();

    for (auto i = 0; i < kwarg_keys->length(); ++i) {
      Handle<PyObject> kwarg_key =
          kwarg_keys->Get(kwarg_keys->length() - i - 1, isolate_);
      Handle<PyObject> actual_arg =
          actual_args->Get(actual_args_size - i - 1, isolate_);

      RETURN_ON_EXCEPTION(
          isolate_, PyDict::Put(kw_args, kwarg_key, actual_arg, isolate_));
    }

    // 从actual_args尾部截去提取出来的参数，剩余部分作为pos_args
    actual_args->ShrinkInternal(actual_args->length() - kwarg_keys->length());
    pos_args = actual_args;
  } else {
    pos_args = actual_args;
  }

  return JustVoid();
}

// private
Handle<PyObject> Interpreter::ReleaseReturnValue() {
  auto result = handle(ret_value_, isolate_);
  ret_value_ = Tagged<PyObject>::null();
  return result;
}

// private
Handle<PyTuple> Interpreter::ReleaseKwArgKeys() {
  auto result = Handle<PyTuple>::cast(handle(kwarg_keys_, isolate_));
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
