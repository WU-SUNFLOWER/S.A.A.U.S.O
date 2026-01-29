// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/frame-object.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/fixed-array.h"
#include "src/objects/py-code-object-klass.h"
#include "src/objects/py-code-object.h"
#include "src/objects/py-dict-views.h"
#include "src/objects/py-dict.h"
#include "src/objects/py-function.h"
#include "src/objects/py-list-klass.h"
#include "src/objects/py-list.h"
#include "src/objects/py-object.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "src/runtime/runtime.h"

namespace saauso::internal {

#define STACK_SIZE() (current_frame_->StackSize())
#define TOP() (current_frame_->TopOfStack())
#define TOP_TAGGED() (current_frame_->TopOfStackTagged())
#define PUSH(elem) (current_frame_->PushToStack(elem))
#define POP() (current_frame_->PopFromStack())
#define POP_TAGGED() (current_frame_->PopFromStackTagged())
#define PEEK(x) (handle(current_frame_->stack()->Get((x))))
#define PEEK_TAGGED(x) (current_frame_->stack()->Get((x)))
#define EMPTY() (STACK_LEVEL() == 0)

void Interpreter::EvalCurrentFrame() {
  uint8_t op_code = 0;
  int op_arg = 0;

#define INTERPRETER_HANDLER(bytecode) handler_##bytecode:

#define DISPATCH()                         \
  do {                                     \
    if (!current_frame_->HasMoreCodes()) { \
      goto exit_interpreter;               \
    }                                      \
    op_code = current_frame_->GetOpCode(); \
    op_arg = current_frame_->GetOpArg();   \
    goto* dispatch_table_[op_code];        \
  } while (0)

#define INTERPRETER_HANDLER_DISPATCH(bytecode, ...) \
  INTERPRETER_HANDLER(bytecode) {                   \
    do {                                            \
      __VA_ARGS__                                   \
    } while (0);                                    \
    DISPATCH();                                     \
  }

#define INTERPRETER_HANDLER_WITH_SCOPE(bytecode, ...) \
  INTERPRETER_HANDLER_DISPATCH(bytecode, HandleScope scope; __VA_ARGS__)

#define INTERPRETER_HANDLER_NOOP(bytecode) \
  INTERPRETER_HANDLER(bytecode) {          \
    DISPATCH();                            \
  }

  if (!dispatch_table_initialized_) {
    for (auto i = 0; i < 256; ++i) {
      dispatch_table_[i] = &&unknown_bytecode;
    }
#define REGISTER_INTERPRETER_HANDLER(bytecode, _) \
  dispatch_table_[bytecode] = &&handler_##bytecode;
    BYTECODE_LIST(REGISTER_INTERPRETER_HANDLER);
#undef REGISTER_INTERPRETER_HANDLER
    dispatch_table_initialized_ = true;
  }

  // 取指执行入口
  DISPATCH();

  INTERPRETER_HANDLER_NOOP(Cache)

  INTERPRETER_HANDLER_DISPATCH(PopTop, POP_TAGGED();)

  INTERPRETER_HANDLER_NOOP(PushNull)

  INTERPRETER_HANDLER_NOOP(EndFor)

  INTERPRETER_HANDLER_NOOP(Nop)

  INTERPRETER_HANDLER_WITH_SCOPE(BinarySubscr, {
    Handle<PyObject> subscr = POP();
    Handle<PyObject> object = POP();
    PUSH(PyObject::Subscr(object, subscr));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreSubscr, {
    Handle<PyObject> subscr = POP();
    Handle<PyObject> object = POP();
    Handle<PyObject> value = POP();
    PyObject::StoreSubscr(object, subscr, value);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(GetIter, { PUSH(PyObject::Iter(POP())); })

  INTERPRETER_HANDLER_DISPATCH(ReturnValue, {
    ret_value_ = POP_TAGGED();
    if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
      goto exit_interpreter;
    }
    LeaveCurrentFrame();
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreName, {
    Handle<PyObject> key = current_frame_->names()->Get(op_arg);
    PyDict::Put(current_frame_->locals(), key, POP());
  })

  INTERPRETER_HANDLER_WITH_SCOPE(UnpackSequence, {
    Handle<PyObject> sequence = POP();
    Handle<PyObject> iterator = PyObject::Iter(sequence);
    auto old_stack_top = current_frame_->stack_top();
    current_frame_->set_stack_top(current_frame_->stack_top() + op_arg);
    while (op_arg-- > 0) {
      current_frame_->stack()->Set(old_stack_top + op_arg,
                                   PyObject::Next(iterator));
    }
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ForIter, {
    Handle<PyObject> iterator = TOP();
    Handle<PyObject> next_result = PyObject::Next(iterator);
    if (next_result.is_null()) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1) + 2);
      POP();
      break;
    }
    PUSH(next_result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreGlobal, {
    Handle<PyObject> key = current_frame_->names()->Get(op_arg);
    PyDict::Put(current_frame_->globals(), key, POP());
  })

  INTERPRETER_HANDLER_WITH_SCOPE(
      LoadConst, { PUSH(current_frame_->consts()->GetTagged(op_arg)); })

  INTERPRETER_HANDLER_WITH_SCOPE(LoadName, {
    Tagged<PyObject> key = current_frame_->names()->GetTagged(op_arg);
    Tagged<PyObject> value = current_frame_->locals()->GetTagged(key);
    if (!value.is_null()) {
      PUSH(value);
      break;
    }

    value = current_frame_->globals()->GetTagged(key);
    if (!value.is_null()) {
      PUSH(value);
      break;
    }

    value = builtins()->GetTagged(key);
    if (!value.is_null()) {
      PUSH(value);
      break;
    }

    std::fprintf(stderr, "NameError: name '%s' is not defined\n",
                 Tagged<PyString>::cast(key)->buffer());
    std::exit(1);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BuildTuple, {
    Handle<PyTuple> tuple = PyTuple::NewInstance(op_arg);
    while (op_arg-- > 0) {
      tuple->SetInternal(op_arg, POP());
    }
    PUSH(tuple);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BuildList, {
    Handle<PyList> list = PyList::NewInstance(op_arg);
    while (op_arg-- > 0) {
      list->SetAndExtendLength(op_arg, POP());
    }
    PUSH(list);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BuildMap, {
    auto result = PyDict::NewInstance();
    for (auto i = 0; i < op_arg; ++i) {
      auto value = POP();
      auto key = POP();
      PyDict::Put(result, key, value);
    }
    PUSH(result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(DictMerge, {
    Handle<PyObject> update = POP();
    Handle<PyObject> target = TOP();
    if (!IsPyDict(*target) || !IsPyDict(*update)) {
      std::fprintf(stderr, "TypeError: DICT_MERGE expected dict operands\n");
      std::exit(1);
    }

    auto target_dict = Handle<PyDict>::cast(target);
    auto update_dict = Handle<PyDict>::cast(update);

    auto iter = PyDictItemIterator::NewInstance(update_dict);
    auto item = Handle<PyTuple>::cast(PyObject::Next(iter));
    while (!item.is_null()) {
      auto key = item->Get(0);
      auto value = item->Get(1);

      if ((op_arg & 1) != 0 && target_dict->Contains(key)) {
        if (IsPyString(*key)) {
          std::fprintf(
              stderr,
              "TypeError: got multiple values for keyword argument '%s'\n",
              Handle<PyString>::cast(key)->buffer());
        } else {
          std::fprintf(stderr,
                       "TypeError: got multiple values for keyword argument\n");
        }
        std::exit(1);
      }

      PyDict::Put(target_dict, key, value);
      item = Handle<PyTuple>::cast(PyObject::Next(iter));
    }
  })

  INTERPRETER_HANDLER_WITH_SCOPE(LoadAttr, {
    Handle<PyObject> object = POP();
    Handle<PyObject> attr_name = current_frame_->names()->Get(op_arg >> 1);
    PUSH(PyObject::GetAttr(object, attr_name));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(CompareOp, {
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
        std::fprintf(stderr, "unknown compare op type: %d\n", compare_op_type);
        std::exit(1);
    }
  })

  INTERPRETER_HANDLER_DISPATCH(JumpForward, {
    current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
  })

  INTERPRETER_HANDLER_DISPATCH(JumpIfFalse, {
    Tagged<PyObject> condition = POP_TAGGED();
    if (!Runtime_PyObjectIsTrue(condition)) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    }
  })

  INTERPRETER_HANDLER_DISPATCH(JumpIfTrue, {
    Tagged<PyObject> condition = POP_TAGGED();
    if (Runtime_PyObjectIsTrue(condition)) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    }
  })

  INTERPRETER_HANDLER_DISPATCH(IsOp, {
    Tagged<PyObject> r = POP_TAGGED();
    Tagged<PyObject> l = POP_TAGGED();
    PUSH(Isolate::ToPyBoolean((l == r) ^ op_arg));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(LoadGlobal, {
    Handle<PyObject> key = current_frame_->names()->Get(op_arg >> 1);

    Handle<PyObject> value = current_frame_->globals()->Get(key);
    if (!value.is_null()) {
      PUSH(value);
      break;
    }

    value = builtins()->Get(key);
    if (!value.is_null()) {
      PUSH(value);
      break;
    }

    std::fprintf(stderr, "NameError: name '%s' is not defined\n",
                 Handle<PyString>::cast(key)->buffer());
    std::exit(1);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ContainsOp, {
    Handle<PyObject> r = POP();
    Handle<PyObject> l = POP();
    bool result = PyObject::Contains(r, l)->value();
    PUSH(Isolate::ToPyBoolean(result ^ op_arg));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BinaryOp, {
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
  })

  INTERPRETER_HANDLER_WITH_SCOPE(
      LoadFast, PUSH(current_frame_->fast_locals()->Get(op_arg));)

  INTERPRETER_HANDLER_WITH_SCOPE(
      StoreFast, current_frame_->fast_locals()->Set(op_arg, *POP());)

  INTERPRETER_HANDLER_WITH_SCOPE(DeleteFast, {
                                                 // todo
                                             })

  INTERPRETER_HANDLER_DISPATCH(PopJumpIfNotNone, {
    if (POP_TAGGED() != Isolate::Current()->py_none_object()) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    }
  })

  INTERPRETER_HANDLER_DISPATCH(PopJumpIfNone, {
    if (POP_TAGGED() == Isolate::Current()->py_none_object()) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    }
  })

  INTERPRETER_HANDLER_DISPATCH(ReturnConst, {
    ret_value_ = current_frame_->code_object()->consts()->GetTagged(op_arg);
    if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
      goto exit_interpreter;
    }
    LeaveCurrentFrame();
  })

  INTERPRETER_HANDLER_WITH_SCOPE(MakeFunction, {
    Handle<PyObject> code_object = POP();
    Handle<PyFunction> func =
        PyFunction::NewInstance(Handle<PyCodeObject>::cast(code_object));
    func->set_func_globals(current_frame_->globals());
    if (op_arg & MakeFunctionOpArgMask::kDefaults) {
      func->set_default_args(Handle<PyTuple>::cast(POP()));
    }
    PUSH(func);
  })

  INTERPRETER_HANDLER_DISPATCH(JumpBackward, {
    current_frame_->set_pc(current_frame_->pc() - (op_arg << 1));
  })

  INTERPRETER_HANDLER_NOOP(Resume)

  INTERPRETER_HANDLER_WITH_SCOPE(BuildConstKeyMap, {
    auto keys = Handle<PyTuple>::cast(POP());
    auto result = PyDict::NewInstance();
    for (auto i = keys->length() - 1; 0 <= i; --i) {
      PyDict::Put(result, keys->Get(i), POP());
    }
    PUSH(result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ListExtend, {
    Handle<PyObject> source = POP();
    Handle<PyObject> list = PEEK(STACK_SIZE() - op_arg);
    Runtime_ExtendListByItratableObject(Handle<PyList>::cast(list), source);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(CallFunctionEx, {
    Handle<PyDict> kw_args;
    if ((op_arg & 1) != 0) {
      Handle<PyObject> kw = POP();
      if (!IsPyDict(*kw)) {
        std::fprintf(
            stderr,
            "TypeError: CALL_FUNCTION_EX expected a dict for **kwargs\n");
        std::exit(1);
      }
      kw_args = Handle<PyDict>::cast(kw);
    }

    Handle<PyObject> args_obj = POP();
    Handle<PyObject> callable = POP();

    Handle<PyTuple> pos_args;
    if (args_obj.is_null()) {
      pos_args = PyTuple::NewInstance(0);
    } else if (IsPyTuple(args_obj)) {
      pos_args = Handle<PyTuple>::cast(args_obj);
    } else {
      pos_args = Runtime_UnpackIterableObjectToTuple(args_obj);
    }

    InvokeCallableWithNormalizedArgs(callable, pos_args, kw_args);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(Call, {
    Handle<PyTuple> args;
    if (op_arg > 0) {
      args = PyTuple::NewInstance(op_arg);
      while (op_arg-- > 0) {
        args->SetInternal(op_arg, POP());
      }
    }
    InvokeCallable(POP(), args, ReleaseKwArgKeys());
  })

  INTERPRETER_HANDLER_DISPATCH(
      KwNames, { kwarg_keys_ = current_frame_->consts()->GetTagged(op_arg); })

#undef INTERPRETER_HANDLER_NOOP
#undef INTERPRETER_HANDLER_WITH_SCOPE
#undef INTERPRETER_HANDLER_DISPATCH
#undef DISPATCH
#undef INTERPRETER_HANDLER

unknown_bytecode:
  std::fprintf(stderr, "unknown bytecode %d, arguments %d\n", op_code, op_arg);
  std::exit(1);

exit_interpreter:
  return;
}

void Interpreter::InvokeCallable(Handle<PyObject> callable,
                                 Handle<PyTuple> actual_args,
                                 Handle<PyTuple> kwarg_keys) {
  Handle<PyObject> host;
  NormalizeCallable(callable, host);

  // Fast Path：如果是普通的python函数，那么直接创建并进入新的解释器栈帧
  if (IsNormalPyFunction(callable)) {
    FrameObject* frame = FrameObject::NewInstance(
        Handle<PyFunction>::cast(callable), host, actual_args, kwarg_keys);
    EnterFrame(frame);
    return;
  }

  // Slow Path：先对用户传入的实参进行归一化，再尝试调用callable的call虚方法
  Handle<PyTuple> pos_args;
  Handle<PyDict> kw_args;
  NormalizeArguments(actual_args, kwarg_keys, pos_args, kw_args);
  Handle<PyObject> result = PyObject::Call(callable, host, pos_args, kw_args);
  PUSH(result);
}

void Interpreter::InvokeCallableWithNormalizedArgs(Handle<PyObject> callable,
                                                   Handle<PyTuple> pos_args,
                                                   Handle<PyDict> kw_args) {
  Handle<PyObject> host;
  NormalizeCallable(callable, host);

  if (IsNormalPyFunction(callable)) {
    FrameObject* frame = FrameObject::NewInstance(
        Handle<PyFunction>::cast(callable), host, pos_args, kw_args);
    EnterFrame(frame);
    return;
  }

  Handle<PyObject> result = PyObject::Call(callable, host, pos_args, kw_args);
  PUSH(result);
}

void Interpreter::EnterFrame(FrameObject* frame) {
  frame->set_caller(current_frame_);
  current_frame_ = frame;
}

void Interpreter::LeaveCurrentFrame() {
  DestroyCurrentFrame();

  // 将被弹出栈帧当中的返回值压入被恢复栈帧的堆栈中
  PUSH(ret_value_);
  ret_value_ = Tagged<PyObject>::null();
}

void Interpreter::DestroyCurrentFrame() {
  FrameObject* callee = current_frame_;
  // 弹出并销毁当前python栈帧，恢复上一个python栈帧
  current_frame_ = callee->caller();
  delete callee;
}

}  // namespace saauso::internal
