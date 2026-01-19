// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/interpreter.h"

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/frame-object.h"
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

  builtins_ = *builtins;
}

Handle<PyObject> Interpreter::builtins() const {
  return handle(builtins_);
}

void Interpreter::Run(Handle<PyCodeObject> code_object) {
  frame_ = new FrameObject(code_object);

  uint8_t op_code = 0;
  int op_arg = 0;

#define INTERPRETER_HANDLER(bytecode) handler_##bytecode:

#define Dispatch()                  \
  do {                              \
    if (!frame_->HasMoreCodes()) {  \
      goto exit;                    \
    }                               \
    op_code = frame_->GetOpCode();  \
    op_arg = frame_->GetOpArg();    \
    goto* dispatch_table_[op_code]; \
  } while (0)

#define REGISTER_INTERPRETER_HANDLER(bytecode, _) \
  dispatch_table_[bytecode] = &&handler_##bytecode;

  if (dispatch_table_initialized_) {
    BYTECODE_LIST(REGISTER_INTERPRETER_HANDLER);
    dispatch_table_initialized_ = true;
  }

  // 取指执行入口
  Dispatch();

  INTERPRETER_HANDLER(PopTop) {
    frame_->PopFromStack();
    Dispatch();
  }

  INTERPRETER_HANDLER(PushNull) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadConst) {
    frame_->PushToStack(frame_->code_object()->consts()->Get(op_arg));
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadName) {
    frame_->PushToStack(frame_->code_object()->names()->Get(op_arg));
    Dispatch();
  }

  INTERPRETER_HANDLER(ReturnConst) {
    // todo
    Dispatch();
  }

  INTERPRETER_HANDLER(Resume) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(Call) {
    // TODO
    Dispatch();
  }

exit:
  return;
}

Handle<PyObject> Interpreter::CallVirtual(Handle<PyObject> callable,
                                          Handle<PyList> args) {
  HandleScope scope;
  Handle<PyList> actual_args = args.IsNull() ? PyList::NewInstance() : args;

  if (IsPyNativeFunction(callable)) {
    return PyObject::Call(callable, args, Handle<PyObject>::Null())
        .EscapeFrom(&scope);
  }

  if (IsPyFunction(callable)) {
    // TODO: 创建新的虚拟机栈帧，执行python代码
    return handle(Isolate::Current()->py_none_object()).EscapeFrom(&scope);
  }

  if (IsMethodObject(callable)) {
    auto method = Handle<MethodObject>::cast(callable);
    PyList::Insert(actual_args, 0, method->owner());
    return CallVirtual(method->func(), actual_args).EscapeFrom(&scope);
  }

  // TypeError: 'int' object is not callable
  std::printf("TypeError: '");
  PyObject::Print(PyObject::GetKlass(callable)->name());
  std::printf("' object is not callable\n");
  std::exit(1);

  return Handle<PyObject>::Null();
}

void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&builtins_);
}

}  // namespace saauso::internal
