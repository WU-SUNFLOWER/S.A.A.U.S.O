// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/frame-object.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "src/runtime/runtime.h"

namespace saauso::internal {

#define STACK_SIZE() (current_frame_->StackSize())
#define TOP() (current_frame_->TopOfStack())
#define PUSH(elem) (current_frame_->PushToStack(elem))
#define POP(elem) (current_frame_->PopFromStack())
#define PEEK(x) (handle(current_frame_->stack()->Get((x))))
#define EMPTY() (STACK_LEVEL() == 0)

void Interpreter::EvalCurrentFrame() {
  uint8_t op_code = 0;
  int op_arg = 0;

#define INTERPRETER_HANDLER(bytecode) handler_##bytecode:

#define Dispatch()                         \
  do {                                     \
    if (!current_frame_->HasMoreCodes()) { \
      goto exit_interpreter;               \
    }                                      \
    op_code = current_frame_->GetOpCode(); \
    op_arg = current_frame_->GetOpArg();   \
    goto* dispatch_table_[op_code];        \
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
    current_frame_->PopFromStack();
    Dispatch();
  }

  INTERPRETER_HANDLER(PushNull) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(EndFor) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(Nop) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(BinarySubscr) {
    do {
      HandleScope scope;
      Handle<PyObject> subscr = POP();
      Handle<PyObject> object = POP();
      PUSH(PyObject::Subscr(object, subscr));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(StoreSubscr) {
    do {
      HandleScope scope;
      Handle<PyObject> subscr = POP();
      Handle<PyObject> object = POP();
      Handle<PyObject> value = POP();
      PyObject::StoreSubscr(object, subscr, value);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(GetIter) {
    do {
      HandleScope scope;
      PUSH(PyObject::Iter(POP()));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(ReturnValue) {
    do {
      {
        HandleScope scope;
        ret_value_ = *POP();
      }
      if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
        // 如果是根栈帧或者是C++栈帧请求的第一个python栈帧，
        // 那么直接退出解释器。
        // 考虑到创建python栈帧的C++栈帧可能需要前者的计算结果（即返回值），
        // 这里我们不去调用LeaveCurrentFrame销毁python栈帧，
        // 而是约定由之前的C++栈帧取出返回值后手动进行销毁！！！
        goto exit_interpreter;
      } else {
        // 否则走正常的栈帧退出路径，退回到上一个python栈帧
        LeaveCurrentFrame();
      }
    } while (0);
    Dispatch();
  }

  // 以names中的值为键，更新locals
  INTERPRETER_HANDLER(StoreName) {
    do {
      HandleScope scope;
      // 从符号表中取出符号
      Handle<PyObject> key = current_frame_->names()->Get(op_arg);
      PyDict::Put(current_frame_->locals(), key, POP());
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(ForIter) {
    do {
      HandleScope scope;
      Handle<PyObject> iterator = TOP();
      Handle<PyObject> next_result = PyObject::Next(iterator);
      if (next_result.IsNull()) {
        current_frame_->set_pc(current_frame_->pc() + (op_arg << 1) + 2);
        POP();  // 弹出iterator
        break;
      }
      PUSH(next_result);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(StoreGlobal) {
    do {
      HandleScope scope;
      // 从符号表中取出符号
      Handle<PyObject> key = current_frame_->names()->Get(op_arg);
      PyDict::Put(current_frame_->globals(), key, POP());
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadConst) {
    do {
      HandleScope scope;
      PUSH(current_frame_->consts()->Get(op_arg));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadName) {
    do {
      HandleScope scope;
      Handle<PyObject> key = current_frame_->names()->Get(op_arg);

      Handle<PyObject> value = current_frame_->locals()->Get(key);
      if (!value.IsNull()) {
        PUSH(value);
        break;
      }

      value = current_frame_->globals()->Get(key);
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

  INTERPRETER_HANDLER(BuildTuple) {
    do {
      HandleScope scope;
      Handle<PyTuple> tuple = PyTuple::NewInstance(op_arg);
      while (op_arg-- > 0) {
        tuple->SetInternal(op_arg, POP());
      }
      PUSH(tuple);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(BuildList) {
    do {
      HandleScope scope;
      Handle<PyList> list = PyList::NewInstance(op_arg);
      while (op_arg-- > 0) {
        list->SetAndExtendLength(op_arg, POP());
      }
      PUSH(list);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(LoadAttr) {
    do {
      HandleScope scope;
      Handle<PyObject> object = POP();
      Handle<PyObject> attr_name = current_frame_->names()->Get(op_arg >> 1);
      PUSH(PyObject::GetAttr(object, attr_name));
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
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpIfFalse) {
    do {
      HandleScope scope;
      if (!Runtime_PyObjectIsTrue(POP())) {
        current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
      }
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpIfTrue) {
    do {
      HandleScope scope;
      if (Runtime_PyObjectIsTrue(POP())) {
        current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
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
      Handle<PyObject> key = current_frame_->names()->Get(op_arg >> 1);

      Handle<PyObject> value = current_frame_->globals()->Get(key);
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

  INTERPRETER_HANDLER(LoadFast) {
    do {
      HandleScope scope;
      PUSH(current_frame_->fast_locals()->Get(op_arg));
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(StoreFast) {
    do {
      HandleScope scope;
      current_frame_->fast_locals()->Set(op_arg, *POP());
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(DeleteFast) {
    do {
      // todo
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(ReturnConst) {
    do {
      {
        HandleScope scope;
        ret_value_ = *current_frame_->code_object()->consts()->Get(op_arg);
      }
      if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
        goto exit_interpreter;
      } else {
        LeaveCurrentFrame();
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
      func->set_func_globals(current_frame_->globals());

      // 为函数体绑定默认参数
      if (op_arg & MakeFunctionOpArgMask::kDefaults) {
        func->set_default_args(Handle<PyTuple>::cast(POP()));
      }

      PUSH(func);
    } while (0);
    Dispatch();
  }

  INTERPRETER_HANDLER(JumpBackward) {
    current_frame_->set_pc(current_frame_->pc() - (op_arg << 1));
    Dispatch();
  }

  INTERPRETER_HANDLER(Resume) {
    // do nothing
    Dispatch();
  }

  INTERPRETER_HANDLER(ListExtend) {
    do {
      HandleScope scope;
      Handle<PyObject> source = POP();
      Handle<PyObject> list = PEEK(STACK_SIZE() - op_arg);

      Handle<PyTuple> args = PyTuple::NewInstance(1);
      args->SetInternal(0, source);
      
      PyListKlass::NativeMethod_Extend(list, args, Handle<PyDict>::Null());
    } while (0);
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

exit_interpreter:
  return;
}

void Interpreter::InvokeCallable(Handle<PyObject> callable,
                                 Handle<PyTuple> args,
                                 Handle<PyDict> kwargs) {
  Handle<PyObject> host;
  NormalizeCallable(callable, host, args, kwargs);

  // 如果是普通的python函数，那么直接创建并进入新的解释器栈帧
  if (IsNormalPyFunction(callable)) {
    FrameObject* frame =
        new FrameObject(Handle<PyFunction>::cast(callable), host, args);
    EnterFrame(frame);
    return;
  }

  // 兜底：尝试调用callable的call虚方法
  Handle<PyObject> result = PyObject::Call(callable, host, args, kwargs);
  PUSH(result);
}

void Interpreter::EnterFrame(FrameObject* frame) {
  frame->set_caller(current_frame_);
  current_frame_ = frame;
}

void Interpreter::LeaveCurrentFrame() {
  DestroyCurrentFrame();

  // 将被弹出栈帧当中的返回值压入被恢复栈帧的堆栈中
  PUSH(ReleaseReturnValue());
}

void Interpreter::DestroyCurrentFrame() {
  FrameObject* callee = current_frame_;
  // 弹出并销毁当前python栈帧，恢复上一个python栈帧
  current_frame_ = callee->caller();
  delete callee;
}

}  // namespace saauso::internal
