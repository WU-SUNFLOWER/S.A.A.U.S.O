// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/interpreter.h"

#include <cstdlib>

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/frame-object.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-klass.h"
#include "src/objects/py-dict.h"
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
#include "src/runtime/isolate.h"
#include "src/runtime/native-functions.h"
#include "src/runtime/runtime.h"
#include "src/runtime/string-table.h"

namespace saauso::internal {

Interpreter::Interpreter() {
  HandleScope scope;

  Handle<PyDict> builtins = PyDict::NewInstance();

  // 注册基础类型的klass
  PyDict::Put(builtins, PyString::NewInstance("object"),
              PyObjectKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("int"),
              PySmiKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("str"),
              PyStringKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("list"),
              PyListKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("bool"),
              PyBooleanKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("dict"),
              PyDictKlass::GetInstance()->type_object());
  PyDict::Put(builtins, PyString::NewInstance("tuple"),
              PyTupleKlass::GetInstance()->type_object());

  // 注册oddballs
  PyDict::Put(builtins, PyString::NewInstance("True"),
              handle(Isolate::Current()->py_true_object()));
  PyDict::Put(builtins, PyString::NewInstance("False"),
              handle(Isolate::Current()->py_false_object()));
  PyDict::Put(builtins, PyString::NewInstance("None"),
              handle(Isolate::Current()->py_none_object()));

  // 注册native function
  auto func_name = PyString::NewInstance("print");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_Print, func_name));
  func_name = PyString::NewInstance("len");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_Len, func_name));

  func_name = PyString::NewInstance("isinstance");
  PyDict::Put(builtins, func_name,
              PyFunction::NewInstance(&Native_IsInstance, func_name));

  // 把builtins自己注册进去
  PyDict::Put(builtins, ST(builtins), builtins);

  builtins_ = *builtins;
}

Handle<PyDict> Interpreter::builtins() const {
  return handle(Tagged<PyDict>::cast(builtins_));
}

void Interpreter::Run(Handle<PyCodeObject> code_object) {
  EnterFrame(FrameObject::NewInstance(code_object));
  EvalCurrentFrame();
  DestroyCurrentFrame();
}

// public
Handle<PyObject> Interpreter::CallPython(Handle<PyObject> callable,
                                         Handle<PyTuple> args,
                                         Handle<PyDict> kwargs) {
  HandleScope scope;

  Handle<PyObject> host;
  NormalizeCallable(callable, host, args, kwargs);

  Handle<PyObject> result;

  // 如果是普通的python函数，那么请求解释器进行处理
  if (IsNormalPyFunction(callable)) {
    FrameObject* frame = FrameObject::NewInstance(
        Handle<PyFunction>::cast(callable), host, args);
    // 确保Python栈帧处理结束后能够退回到C++栈帧
    frame->set_is_entry_frame(true);

    // 调用解释器处理栈帧
    EnterFrame(frame);
    EvalCurrentFrame();

    // 手工提取返回值，然后弹出当前python栈帧
    result = ReleaseReturnValue();
    DestroyCurrentFrame();

    return result;
  }

  // 兜底：尝试调用callable的call虚方法
  result = PyObject::Call(callable, host, args, kwargs);
  return result;
}

// private
void Interpreter::NormalizeCallable(Handle<PyObject>& callable,
                                    Handle<PyObject>& host,
                                    Handle<PyTuple>& args,
                                    Handle<PyDict>& kwargs) {
  // 如果是对象方法，那么进行解包
  if (IsMethodObject(callable)) {
    auto method = Handle<MethodObject>::cast(callable);
    callable = method->func();
    host = method->owner();
  }
}

// private
Handle<PyObject> Interpreter::ReleaseReturnValue() {
  auto result = handle(ret_value_);
  ret_value_ = Tagged<PyObject>::Null();
  return result;
}

// public
void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&builtins_);
  v->VisitPointer(&ret_value_);
  v->VisitPointer(&kwargs_);

  FrameObject* frame = current_frame_;
  while (frame != nullptr) {
    frame->Iterate(v);
    frame = current_frame_->caller();
  }
}

}  // namespace saauso::internal
