// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/interpreter/interpreter.h"

#include <cstdlib>

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

#define PUSH(elem) (frame_->PushToStack(elem))
#define POP(elem) (frame_->PopFromStack())

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

  if (!dispatch_table_initialized_) {
    for (auto i = 0; i < 256; ++i) {
      dispatch_table_[i] = &&unknown_bytecode;
    }
    BYTECODE_LIST(REGISTER_INTERPRETER_HANDLER);
    dispatch_table_initialized_ = true;
  }

  // 取指执行入口
  Dispatch();

  INTERPRETER_HANDLER(Cache) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(PopTop) {
    frame_->PopFromStack();
    Dispatch();
  }

  INTERPRETER_HANDLER(PushNull) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(Nop) {
    // do nothing
    Dispatch();
  }

  // 以names中的值为键，更新locals
  INTERPRETER_HANDLER(StoreName) {
    do {
      HandleScope scope;
      // 从符号表中取出符号
      Handle<PyObject> key = frame_->names()->Get(op_arg);
      PyDict::Put(frame_->locals(), key, POP());
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(StoreGlobal) {
    do {
      HandleScope scope;
      // 从符号表中取出符号
      Handle<PyObject> key = frame_->names()->Get(op_arg);
      PyDict::Put(frame_->globals(), key, POP());
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadConst) {
    do {
      HandleScope scope;
      PUSH(frame_->consts()->Get(op_arg));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadName) {
    do {
      HandleScope scope;
      Handle<PyObject> key = frame_->names()->Get(op_arg);

      Handle<PyObject> value = frame_->locals()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      value = frame_->globals()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      value = builtins()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      std::printf("NameError: name '");
      PyObject::Print(key);
      std::printf("' is not defined\n");
      std::exit(1);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(CompareOp) {
    do {
      HandleScope scope;
      Handle<PyObject> r = POP();
      Handle<PyObject> l = POP();
      int compare_op_type = op_arg >> 4;
      switch (compare_op_type) {
        case CompareOpType::kLT:
          PUSH(PyObject::Less(l, r));
          break;
        case CompareOpType::kLE:
          PUSH(PyObject::LessEqual(l, r));
          break;
        case CompareOpType::kEQ:
          PUSH(PyObject::Equal(l, r));
          break;
        case CompareOpType::kNE:
          PUSH(PyObject::NotEqual(l, r));
          break;
        case CompareOpType::kGT:
          PUSH(PyObject::Greater(l, r));
          break;
        case CompareOpType::kGE:
          PUSH(PyObject::GreaterEqual(l, r));
          break;
        default:
          std::printf("unknown compare op type: %d\n", compare_op_type);
          std::exit(1);
      }
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpForward) {
    do {
      frame_->set_pc(frame_->pc() + (op_arg << 1));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpIfFalse) {
    do {
      HandleScope scope;
      if (!Runtime_PyObjectIsTrue(POP())) {
        frame_->set_pc(frame_->pc() + (op_arg << 1));
      }
    } while (0);
    Dispatch();
  }


  INTERPRETER_HANDLER(JumpIfTrue) {
    do {
      HandleScope scope;
      if (Runtime_PyObjectIsTrue(POP())) {
        frame_->set_pc(frame_->pc() + (op_arg << 1));
      }
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(IsOp) {
    do {
      HandleScope scope;
      Handle<PyObject> r = POP();
      Handle<PyObject> l = POP();
      PUSH(Isolate::ToPyBoolean(l.is_identical_to(r) ^ op_arg));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadGlobal) {
    do {
      HandleScope scope;
      Handle<PyObject> key = frame_->names()->Get(op_arg >> 1);

      Handle<PyObject> value = frame_->globals()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      value = builtins()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      std::printf("NameError: name '");
      PyObject::Print(key);
      std::printf("' is not defined\n");
      std::exit(1);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(ContainsOp) {
    do {
      HandleScope scope;
      // l in r
      Handle<PyObject> r = POP();
      Handle<PyObject> l = POP();
      // 注意这里的参数顺序，和一般的运算符正好是反的
      bool result = PyObject::Contains(r, l)->value();
      PUSH(Isolate::ToPyBoolean(result ^ op_arg));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(BinaryOp) {
    do {
      HandleScope scope;
      Handle<PyObject> r = POP();
      Handle<PyObject> l = POP();
      switch (op_arg) {
        case BinaryOpType::kAdd:
        case BinaryOpType::kInplaceAdd:
          PUSH(PyObject::Add(l, r));
          break;
        case BinaryOpType::kSubtract:
        case BinaryOpType::kInplaceSubtract:
          PUSH(PyObject::Sub(l, r));
          break;
        case BinaryOpType::kMultiply:
        case BinaryOpType::kInplaceMultiply:
          PUSH(PyObject::Mul(l, r));
          break;
        case BinaryOpType::kTrueDivide:
        case BinaryOpType::kInplaceTrueDivide:
          PUSH(PyObject::Div(l, r));
          break;
        case BinaryOpType::kRemainder:
        case BinaryOpType::kInplaceRemainder:
          PUSH(PyObject::Mod(l, r));
          break;
      }
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(ReturnConst) {
    do {
      ret_value_ = *frame_->code_object()->consts()->Get(op_arg);
      if (frame_->IsFirstFrame()) {
        goto exit;
      } else {
        LeaveFrame();
      }
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(MakeFunction) {
    do {
      HandleScope scope;
      Handle<PyObject> code_object = POP();
      Handle<PyFunction> func =
          PyFunction::NewInstance(Handle<PyCodeObject>::cast(code_object));
      // 将当前栈帧的全局变量表绑定到新创建的函数体
      func->set_func_globals(frame_->globals());
      PUSH(func);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpBackward) {
    frame_->set_pc(frame_->pc() - (op_arg << 1));
    Dispatch();
  }

  INTERPRETER_HANDLER(Resume) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(Call) {
    do {
      HandleScope scope;
      Handle<PyTuple> args;
      if (op_arg > 0) {
        args = PyTuple::NewInstance(op_arg);
        while (op_arg-- > 0) {
          args->SetInternal(op_arg, POP());
        }
      }
      InvokeCallable(POP(), args, Handle<PyDict>::Null());
    } while (0);
    Dispatch();
  }

unknown_bytecode:
  std::printf("unknown bytecode %d, arguments %d\n", op_code, op_arg);
  std::exit(1);

exit:
  return;
}

void Interpreter::InvokeCallable(Handle<PyObject> callable,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  if (IsNormalPyFunction(callable)) {
    BuildFrame(Handle<PyFunction>::cast(callable));
  } else {
    Handle<PyObject> result = PyObject::Call(callable, args, kwargs);
    PUSH(result);
  }
}

void Interpreter::BuildFrame(Handle<PyFunction> func) {
  FrameObject* frame = new FrameObject(func);
  frame->set_caller(frame_);
  frame_ = frame;
}

void Interpreter::LeaveFrame() {
  DestroyFrame();
  PUSH(ret_value_);
  ret_value_ = Tagged<PyObject>::Null();
}

void Interpreter::DestroyFrame() {
  FrameObject* callee = frame_;
  frame_ = callee->caller();
  delete callee;
}

Handle<PyObject> Interpreter::CallVirtual(Handle<PyObject> callable,
                                          Handle<PyTuple> args) {
  // HandleScope scope;
  // auto actual_args = args.IsNull() ? PyTuple::NewInstance() : args;

  // if (IsPyNativeFunction(callable)) {
  //   return PyObject::Call(callable, args, Handle<PyObject>::Null())
  //       .EscapeFrom(&scope);
  // }

  // if (IsPyFunction(callable)) {
  //   // TODO: 创建新的虚拟机栈帧，执行python代码
  //   return handle(Isolate::Current()->py_none_object()).EscapeFrom(&scope);
  // }

  // if (IsMethodObject(callable)) {
  //   auto method = Handle<MethodObject>::cast(callable);
  //   PyList::Insert(actual_args, 0, method->owner());
  //   return CallVirtual(method->func(), actual_args).EscapeFrom(&scope);
  // }

  // // TypeError: 'int' object is not callable
  // std::printf("TypeError: '");
  // PyObject::Print(PyObject::GetKlass(callable)->name());
  // std::printf("' object is not callable\n");
  // std::exit(1);

  return Handle<PyObject>::Null();
}

void Interpreter::Iterate(ObjectVisitor* v) {
  v->VisitPointer(&builtins_);
}

}  // namespace saauso::internal
