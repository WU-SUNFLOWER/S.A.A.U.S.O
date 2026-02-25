// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/execution/exception-utils.h"
#include "src/execution/execution.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/exception-table.h"
#include "src/interpreter/frame-object-builder.h"
#include "src/interpreter/frame-object.h"
#include "src/interpreter/interpreter.h"
#include "src/modules/module-manager.h"
#include "src/objects/cell.h"
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
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/objects/py-type-object.h"
#include "src/runtime/runtime-exceptions.h"
#include "src/runtime/runtime-intrinsics.h"
#include "src/runtime/runtime-iterable.h"
#include "src/runtime/runtime-reflection.h"
#include "src/runtime/runtime-truthiness.h"
#include "src/runtime/string-table.h"

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

#define DISPATCH()                                      \
  do {                                                  \
    if (isolate_->HasPendingException()) [[unlikely]] { \
      goto pending_exception_unwind;                    \
    }                                                   \
    if (!current_frame_->HasMoreCodes()) [[unlikely]] { \
      goto exit_interpreter;                            \
    }                                                   \
    op_code = current_frame_->GetOpCode();              \
    op_arg = current_frame_->GetOpArg();                \
    goto* dispatch_table_[op_code];                     \
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

#define ASSIGN_GOTO_ON_EXCEPTION(dst, call) \
  ASSIGN_GOTO_ON_EXCEPTION_TARGET(isolate_, dst, call, pending_exception_unwind)

#define GOTO_ON_EXCEPTION(call) \
  GOTO_ON_EXCEPTION_TARGET(isolate_, call, pending_exception_unwind)

    dispatch_table_initialized_ = true;
  }

  // 取指执行入口
  DISPATCH();

  INTERPRETER_HANDLER_NOOP(Cache)

  INTERPRETER_HANDLER_DISPATCH(PopTop, POP_TAGGED();)

  INTERPRETER_HANDLER_DISPATCH(PushNull, { PUSH(Tagged<PyObject>::null()); })

  INTERPRETER_HANDLER_NOOP(EndFor)

  INTERPRETER_HANDLER_NOOP(Nop)

  INTERPRETER_HANDLER_DISPATCH(Copy, {
    assert(op_arg > 0);
    const int index = current_frame_->stack_top() - op_arg;
    PUSH(current_frame_->StackGetTagged(index));
  })

  INTERPRETER_HANDLER_DISPATCH(Swap, {
    assert(op_arg > 0);
    auto stack = current_frame_->stack();
    const int index = current_frame_->stack_top() - op_arg;
    const int top = current_frame_->stack_top() - 1;
    Tagged<PyObject> a = stack->Get(index);
    Tagged<PyObject> b = stack->Get(top);
    stack->Set(index, b);
    stack->Set(top, a);
  })

  // 进入 except handler时，把“当前捕获异常状态”与“新异常对象”一起写入栈，
  // 并更新解释器的caught_exception_，以便后续 `raise`(无参) 可以再抛该异常。
  //
  // 操作数栈结构:
  //   before: [..., exc]
  //   after : [..., prev_exc, exc]
  INTERPRETER_HANDLER_WITH_SCOPE(PushExcInfo, {
    Handle<PyObject> new_exception = POP();

    // 保存更旧的exception的信息
    PUSH(caught_exception_);
    caught_exception_origin_pc_stack_.PushBack(caught_exception_origin_pc_);

    // 将当前最新的exception重新压回栈顶，并更新caught_exception_
    // 和caught_exception_origin_pc_，以便接下来RaiseVarargs
    // 字节码可以无参数场景下正确处理之。
    PUSH(new_exception);
    caught_exception_ = *new_exception;
    caught_exception_origin_pc_ = stack_exception_origin_pc_;
  })

  // 离开 except handler 时恢复 caught_exception_。
  //
  // 操作数栈结构:
  //   before: [..., prev_exc]
  //   after : [...]
  INTERPRETER_HANDLER_WITH_SCOPE(PopExcept, {
    // 注意：在 CPython 生成的 except handler 代码序列中，
    // new exception 会在 handler 内被 POP_TOP 清理掉，因此此处栈顶保存的是
    // prev_exc。
    Handle<PyObject> prev_exc = POP();
    caught_exception_ =
        prev_exc.is_null() ? Tagged<PyObject>::null() : *prev_exc;
    if (!caught_exception_origin_pc_stack_.IsEmpty()) {
      caught_exception_origin_pc_ = caught_exception_origin_pc_stack_.GetBack();
      caught_exception_origin_pc_stack_.PopBack();
    } else {
      caught_exception_origin_pc_ = ExceptionState::kInvalidProgramCounter;
    }
  })

  // 判断异常对象是否匹配某个异常类型（或 tuple-of-types），并把bool压栈。
  // match_type 不合法时，这里就地构造一个TypeError向前传播。
  //
  // 操作数栈结构:
  //   before: [..., exc, match_type]
  //   after : [..., exc, bool]
  INTERPRETER_HANDLER_WITH_SCOPE(CheckExcMatch, {
    // 消费压到栈顶的目标match_type，如ValueError，RuntimeError等
    Handle<PyObject> match_type = POP();
    // 在Python3.12中，待匹配的实际exception在本字节码中只访问不消费。
    // - 一般情况下，**若成功匹配**，则它会被后续的一条PopTop字节码消费掉；
    // - 如果最终没有匹配到当前的任何except块，它就会被保留在操作数栈上，
    //   以便于解释器进一步借助Reraise字节码继续向上传播该异常。
    Handle<PyObject> exception = TOP();

    bool matched = false;
    if (IsPyTypeObject(match_type)) {
      matched = Runtime_IsInstanceOfTypeObject(
          exception, Handle<PyTypeObject>::cast(match_type));
    } else if (IsPyTuple(match_type)) {
      auto tuple = Handle<PyTuple>::cast(match_type);

      // 对齐原版Python行为：
      // 整个tuple都需要完整遍历一遍，
      // 如果出现非法的elem，则直接抛出错误！
      for (auto i = 0; i < tuple->length(); ++i) {
        auto elem = tuple->Get(i);

        if (IsPyTypeObject(elem)) [[likely]] {
          matched |= Runtime_IsInstanceOfTypeObject(
              exception, Handle<PyTypeObject>::cast(elem));
          continue;
        }

        auto type_error_type =
            Handle<PyTypeObject>::cast(builtins()->Get(ST(type_err)));
        Handle<PyObject> type_error;

        ASSIGN_GOTO_ON_EXCEPTION(
            type_error,
            Runtime_NewObject(type_error_type, Handle<PyObject>::null(),
                              Handle<PyObject>::null()));

        const int raise_pc = current_frame_->pc() - kBytecodeSizeInBytes;
        isolate_->exception_state()->Throw(*type_error);
        isolate_->exception_state()->set_pending_exception_pc(raise_pc);
        isolate_->exception_state()->set_pending_exception_origin_pc(raise_pc);
        goto pending_exception_unwind;
      }
    } else {
      auto type_error_type =
          Handle<PyTypeObject>::cast(builtins()->Get(ST(type_err)));
      Handle<PyObject> type_error;

      ASSIGN_GOTO_ON_EXCEPTION(
          type_error,
          Runtime_NewObject(type_error_type, Handle<PyObject>::null(),
                            Handle<PyObject>::null()));

      const int raise_pc = current_frame_->pc() - kBytecodeSizeInBytes;
      isolate_->exception_state()->Throw(*type_error);
      isolate_->exception_state()->set_pending_exception_pc(raise_pc);
      isolate_->exception_state()->set_pending_exception_origin_pc(raise_pc);
      goto pending_exception_unwind;
    }

    PUSH(isolate_->ToPyBoolean(matched));
  })

  // 设置 pending_exception_ 并触发异常展开（由 DISPATCH 统一跳转到
  // pending_exception_unwind）。
  // - oparg == 0 : raise（无参数）→ 重抛 caught_exception_；
  //                                若为空则抛RuntimeError
  // - oparg == 1 : raise exc
  // - oparg == 2 : raise exc from cause（MVP：记录 cause 但不实现链路）
  //
  // 注意：
  // - pending_exception_pc_ 表示“当前 raise 指令”的位置，用于 handler 查找；
  // - pending_exception_origin_pc_ 表示“异常最初发生点”，用于 push-lasti语义。
  INTERPRETER_HANDLER_WITH_SCOPE(RaiseVarargs, {
    Handle<PyObject> cause;
    Handle<PyObject> exception;

    // raise 指令所在的地址
    const int raise_pc = current_frame_->pc() - kBytecodeSizeInBytes;
    isolate_->exception_state()->set_pending_exception_pc(raise_pc);

    switch (op_arg) {
      case 0:
        if (!caught_exception_.is_null()) [[likely]] {
          exception = handle(caught_exception_);
          if (caught_exception_origin_pc_ ==
              ExceptionState::kInvalidProgramCounter) [[unlikely]] {
            isolate_->exception_state()->set_pending_exception_origin_pc(
                raise_pc);
          }
        } else {
          auto runtime_error_type =
              Handle<PyTypeObject>::cast(builtins()->Get(ST(runtime_err)));

          ASSIGN_GOTO_ON_EXCEPTION(
              exception,
              Runtime_NewObject(runtime_error_type, Handle<PyObject>::null(),
                                Handle<PyObject>::null()));

          isolate_->exception_state()->set_pending_exception_origin_pc(
              raise_pc);
        }
        break;
      case 1:
        exception = POP();
        isolate_->exception_state()->set_pending_exception_origin_pc(raise_pc);
        break;
      case 2:
        cause = POP();
        exception = POP();
        isolate_->exception_state()->set_pending_exception_origin_pc(raise_pc);
        break;
      default:
        Runtime_ThrowErrorf(ExceptionType::kTypeError,
                            "invalid RAISE_VARARGS oparg=%d", op_arg);
        goto pending_exception_unwind;
    }

    // 如果用户抛出的是类似于ValueError、RuntimeError之类的type object，
    // 我们需要将其实例化。
    if (IsPyTypeObject(exception)) {
      auto type_object = Handle<PyTypeObject>::cast(exception);
      ASSIGN_GOTO_ON_EXCEPTION(
          exception, Runtime_NewObject(type_object, Handle<PyObject>::null(),
                                       Handle<PyObject>::null()));
    }

    isolate_->exception_state()->Throw(*exception);
  })

  // 如果没匹配任何 except，重新抛出异常
  // - oparg == 0：使用 caught_exception_origin_pc_ 作为 origin pc（用于
  // push-lasti）
  // - oparg != 0：从栈中弹出 lasti（code units），并将其作为 origin pc（用于
  // push-lasti）
  //
  // 操作数栈结构:
  //   oparg == 0 : before [..., exc]            after [...]
  //   oparg != 0 : before [..., lasti, exc]     after [...]
  INTERPRETER_HANDLER_WITH_SCOPE(Reraise, {
    Handle<PyObject> exception = POP();

    const int raise_pc = current_frame_->pc() - kBytecodeSizeInBytes;
    isolate_->exception_state()->set_pending_exception_pc(raise_pc);
    isolate_->exception_state()->set_pending_exception_origin_pc(raise_pc);

    if (op_arg != 0) {
      Handle<PyObject> lasti_obj = POP();
      if (IsPySmi(lasti_obj)) {
        int lasti = Tagged<PySmi>::cast(*lasti_obj).value();
        isolate_->exception_state()->set_pending_exception_origin_pc(lasti);
      }
    } else {
      if (caught_exception_origin_pc_ !=
          ExceptionState::kInvalidProgramCounter) {
        isolate_->exception_state()->set_pending_exception_origin_pc(
            caught_exception_origin_pc_);
      }
    }

    isolate_->exception_state()->Throw(*exception);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BinarySubscr, {
    Handle<PyObject> subscr = POP();
    Handle<PyObject> object = POP();
    Handle<PyObject> result;
    ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Subscr(object, subscr));
    PUSH(result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreSubscr, {
    Handle<PyObject> subscr = POP();
    Handle<PyObject> object = POP();
    Handle<PyObject> value = POP();
    GOTO_ON_EXCEPTION(PyObject::StoreSubscr(object, subscr, value));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(DeleteSubscr, {
    Handle<PyObject> subscr = POP();
    Handle<PyObject> object = POP();
    GOTO_ON_EXCEPTION(PyObject::DeletSubscr(object, subscr));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(GetIter, {
    Handle<PyObject> iterable = POP();
    Handle<PyObject> iterator;
    ASSIGN_GOTO_ON_EXCEPTION(iterator, PyObject::Iter(iterable));
    PUSH(iterator);
  })

  INTERPRETER_HANDLER_DISPATCH(LoadBuildClass, {
    PUSH(builtins_tagged()->Get(ST_TAGGED(func_build_class)));
  })

  INTERPRETER_HANDLER_DISPATCH(ReturnValue, {
    ret_value_ = POP_TAGGED();
    if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
      goto exit_interpreter;
    }
    LeaveCurrentFrame();
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreName, {
    Handle<PyObject> key = current_frame_->names()->Get(op_arg);

    Handle<PyDict> locals = current_frame_->locals();
    // Python中栈帧的locals是按需创建的。
    // 我们不保证一个函数栈帧当中一定有一个有效的locals字典！
    if (locals.is_null()) [[unlikely]] {
      Runtime_ThrowError(ExceptionType::kRuntimeError,
                         "no locals found when storing name");
      goto pending_exception_unwind;
    }

    PyDict::Put(locals, key, POP());
  })

  // 删除一个 name（用于 except ... as e
  // 的清理阶段，确保异常变量不泄漏到外层作用域）。
  INTERPRETER_HANDLER_WITH_SCOPE(DeleteName, {
    Handle<PyObject> key = current_frame_->names()->Get(op_arg);

    Handle<PyDict> locals = current_frame_->locals();
    if (locals.is_null()) {
      break;
    }
    locals->Remove(key);
  })

  // CPython 约定：UNPACK_SEQUENCE 后栈顶为可迭代对象的第一个元素，以便后续
  // 第一个 STORE_FAST 弹出栈顶赋给第一个变量，从而 (k, v) = item 得到
  // k=item[0], v=item[1]。
  INTERPRETER_HANDLER_WITH_SCOPE(UnpackSequence, {
    Handle<PyObject> sequence = POP();
    Handle<PyTuple> tuple;
    ASSIGN_GOTO_ON_EXCEPTION(tuple,
                             Runtime_UnpackIterableObjectToTuple(sequence));
    if (tuple->length() != op_arg) {
      Runtime_ThrowErrorf(ExceptionType::kValueError,
                          "unpack expected %d values, got %d", op_arg,
                          static_cast<int>(tuple->length()));
      goto pending_exception_unwind;
    }
    auto old_stack_top = current_frame_->stack_top();
    current_frame_->set_stack_top(old_stack_top + op_arg);
    for (int i = 0; i < op_arg; ++i) {
      current_frame_->stack()->Set(old_stack_top + (op_arg - 1 - i),
                                   tuple->Get(i));
    }
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ForIter, {
    Handle<PyObject> iterator = TOP();
    Handle<PyObject> next_result;
    MaybeHandle<PyObject> next_maybe = PyObject::Next(iterator);

    // Happy Path: 迭代得到一个有效值，遂直接压栈并退出bytecode handler
    if (next_maybe.ToHandle(&next_result)) [[likely]] {
      PUSH(next_result);
      break;
    }

    assert(isolate_->HasPendingException());

    // 如果是 StopIteration，那么就地消费异常，然后直接跳出迭代块。
    if (Runtime_ConsumePendingStopIterationIfSet(isolate_)) [[likely]] {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1) + 2);
      POP();
      break;
    }
  })

  INTERPRETER_HANDLER_WITH_SCOPE(StoreAttr, {
    Handle<PyObject> attr_name = current_frame_->names()->Get(op_arg);
    Handle<PyObject> object = POP();
    Handle<PyObject> attr_value = POP();
    GOTO_ON_EXCEPTION(PyObject::SetAttr(object, attr_name, attr_value));
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

    Runtime_ThrowErrorf(ExceptionType::kNameError, "name '%s' is not defined",
                        Tagged<PyString>::cast(key)->buffer());
    goto pending_exception_unwind;
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
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "DICT_MERGE expected dict operands");
      goto pending_exception_unwind;
    }

    auto target_dict = Handle<PyDict>::cast(target);
    auto update_dict = Handle<PyDict>::cast(update);

    auto iter = PyDictItemIterator::NewInstance(update_dict);
    while (true) {
      Handle<PyObject> item_handle;
      if (!PyObject::Next(iter).ToHandle(&item_handle)) {
        if (!Runtime_ConsumePendingStopIterationIfSet(isolate_)) {
          goto pending_exception_unwind;
        }
        break;
      }

      auto item = Handle<PyTuple>::cast(item_handle);
      auto key = item->Get(0);
      auto value = item->Get(1);

      if ((op_arg & 1) != 0 && target_dict->Contains(key)) {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "got multiple values for keyword argument");
        goto pending_exception_unwind;
      }

      PyDict::Put(target_dict, key, value);
    }
  })

  INTERPRETER_HANDLER_WITH_SCOPE(LoadAttr, {
    Handle<PyObject> object = POP();
    Handle<PyObject> attr_name = current_frame_->names()->Get(op_arg >> 1);

    // 在python代码中出现了类似于`obj.do_something(arg)`的操作，
    // 即此处可能发生了对象方法调用。
    // 尝试直接查询目标方法（裸的PyFunction）并随对象自身一起压到栈上，
    // 这样就可以避免创建临时的MethodObject对象！！！
    if ((op_arg & 1) != 0) {
      Handle<PyObject> self_or_null;
      Handle<PyObject> value;
      if (!PyObject::GetAttrForCall(object, attr_name, self_or_null)
               .ToHandle(&value)) {
        goto pending_exception_unwind;
      }
      if (!self_or_null.is_null()) {
        // Happy case: attr_name的确对应一个对象方法
        PUSH(value);
        PUSH(self_or_null);
      } else {
        // Bad case: attr_name不是一个对象方法，
        // 这时按照常规的函数调用协议压栈。
        PUSH(Tagged<PyObject>::null());
        PUSH(value);
      }
      break;
    }

    // 一般的属性获取，走常规的GetAttr虚函数查询
    Handle<PyObject> value;
    ASSIGN_GOTO_ON_EXCEPTION(value, PyObject::GetAttr(object, attr_name));
    PUSH(value);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(CompareOp, {
    Handle<PyObject> r = POP();
    Handle<PyObject> l = POP();
    Handle<PyObject> result;
    int compare_op_type = op_arg >> 4;
    switch (compare_op_type) {
      case CompareOpType::kLT:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Less(l, r));
        break;
      case CompareOpType::kLE:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::LessEqual(l, r));
        break;
      case CompareOpType::kEQ:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Equal(l, r));
        break;
      case CompareOpType::kNE:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::NotEqual(l, r));
        break;
      case CompareOpType::kGT:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Greater(l, r));
        break;
      case CompareOpType::kGE:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::GreaterEqual(l, r));
        break;
      default:
        Runtime_ThrowErrorf(ExceptionType::kRuntimeError,
                            "unknown compare op type: %d", compare_op_type);
        goto pending_exception_unwind;
    }
    PUSH(result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ImportName, {
    // 获取导入列表。
    // 例如，`from os import path, rmdir`的fromlist是
    // ('path', 'rmdir')。
    auto fromlist_obj = POP();
    Handle<PyTuple> fromlist;
    // 对于纯import操作，fromlist为None。
    // 例如`import os`就没有有效的fromlist。
    if (!IsPyNone(fromlist_obj)) {
      fromlist = Handle<PyTuple>::cast(fromlist_obj);
    }

    // 相对导入层级。
    // 例如`import os`和`from os import path`的level都是0。
    // `from .os import path`的level是1。
    // `from ..os import path`的level是2。
    auto level = Handle<PySmi>::cast(POP());

    auto name = Handle<PyString>::cast(current_frame_->names()->Get(op_arg));

    Handle<PyObject> module;
    ASSIGN_GOTO_ON_EXCEPTION_TARGET(
        isolate_, module,
        isolate_->module_manager()->ImportModule(
            name, fromlist, PySmi::ToInt(level), current_frame_->globals()),
        pending_exception_unwind);
    PUSH(module);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ImportFrom, {
    auto parent_module = TOP();
    auto sub_module_name =
        Handle<PyString>::cast(current_frame_->names()->Get(op_arg));

    // Fast Path: 如果目标子模块已经被解析过了，直接返回（Try 语义用
    // LookupAttr）
    Handle<PyObject> value;
    if (PyObject::LookupAttr(parent_module, sub_module_name, value) &&
        !value.is_null()) {
      PUSH(value);
      break;
    }
    if (isolate_->HasPendingException()) {
      goto pending_exception_unwind;
    }

    // Slow Path: 目标子模块还没被解析过，走完整的解析流程

    Handle<PyObject> parent_module_name_obj;
    ASSIGN_GOTO_ON_EXCEPTION(parent_module_name_obj,
                             PyObject::GetAttr(parent_module, ST(name)));
    if (!IsPyString(parent_module_name_obj)) [[unlikely]] {
      Runtime_ThrowError(ExceptionType::kTypeError,
                         "module __name__ must be a string");
      goto pending_exception_unwind;
    }

    auto parent_module_name = Handle<PyString>::cast(parent_module_name_obj);

    // 拼接目标子模块的完整符号名
    // 如`from os import path`，则目标子模块`path`的
    // 完整符号名为`os.path`。
    auto sub_module_fullname = PyString::Clone(parent_module_name);
    sub_module_fullname = PyString::Append(sub_module_fullname, ST(dot));
    sub_module_fullname =
        PyString::Append(sub_module_fullname, sub_module_name);

    auto synthetic_fromlist = PyTuple::NewInstance(1);
    synthetic_fromlist->SetInternal(0, sub_module_name);

    Handle<PyObject> submodule;
    ASSIGN_GOTO_ON_EXCEPTION_TARGET(isolate_, submodule,
                                    isolate_->module_manager()->ImportModule(
                                        sub_module_fullname, synthetic_fromlist,
                                        0, current_frame_->globals()),
                                    pending_exception_unwind);
    PUSH(submodule);
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
    if ((op_arg & 1) != 0) {
      PUSH(Tagged<PyObject>::null());
    }

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

    Runtime_ThrowErrorf(ExceptionType::kNameError, "name '%s' is not defined",
                        Handle<PyString>::cast(key)->buffer());
    goto pending_exception_unwind;
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ContainsOp, {
    Handle<PyObject> r = POP();
    Handle<PyObject> l = POP();
    bool result = false;
    if (!PyObject::ContainsBool(r, l).To(&result)) {
      goto pending_exception_unwind;
    }
    PUSH(Isolate::ToPyBoolean(result ^ op_arg));
  })

  INTERPRETER_HANDLER_WITH_SCOPE(BinaryOp, {
    Handle<PyObject> r = POP();
    Handle<PyObject> l = POP();
    Handle<PyObject> result;
    switch (op_arg) {
      case BinaryOpType::kAdd:
      case BinaryOpType::kInplaceAdd:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Add(l, r));
        break;
      case BinaryOpType::kSubtract:
      case BinaryOpType::kInplaceSubtract:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Sub(l, r));
        break;
      case BinaryOpType::kMultiply:
      case BinaryOpType::kInplaceMultiply:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Mul(l, r));
        break;
      case BinaryOpType::kTrueDivide:
      case BinaryOpType::kInplaceTrueDivide:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Div(l, r));
        break;
      case BinaryOpType::kFloorDivide:
      case BinaryOpType::kInplaceFloorDivide:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::FloorDiv(l, r));
        break;
      case BinaryOpType::kRemainder:
      case BinaryOpType::kInplaceRemainder:
        ASSIGN_GOTO_ON_EXCEPTION(result, PyObject::Mod(l, r));
        break;
    }
    PUSH(result);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(
      LoadFast, PUSH(current_frame_->localsplus()->Get(op_arg));)

  INTERPRETER_HANDLER_WITH_SCOPE(
      StoreFast, current_frame_->localsplus()->Set(op_arg, *POP());)

  INTERPRETER_HANDLER_WITH_SCOPE(DeleteFast, {
                                                 // todo
                                             })

  INTERPRETER_HANDLER_DISPATCH(PopJumpIfNotNone, {
    if (POP_TAGGED() != isolate_->py_none_object()) {
      current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
    }
  })

  INTERPRETER_HANDLER_DISPATCH(PopJumpIfNone, {
    if (POP_TAGGED() == isolate_->py_none_object()) {
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
    // 向函数对象注入创建它的函数所绑定的全局变量表
    // 这是Python中函数依据词法作用域规则访问它所在模块全局变量的根本原理！！！
    func->set_func_globals(current_frame_->globals());

    // 向函数对象中注入依据词法作用域捕获到的cell们
    if (op_arg & MakeFunctionOpArgMask::kClosure) {
      func->set_closures(Handle<PyTuple>::cast(POP()));
    }

    // 向函数对象中注入形参默认值
    if (op_arg & MakeFunctionOpArgMask::kDefaults) {
      func->set_default_args(Handle<PyTuple>::cast(POP()));
    }

    PUSH(func);
  })

  // 初始化闭包变量
  INTERPRETER_HANDLER_WITH_SCOPE(MakeCell, {
    Handle<Cell> cell = Cell::NewInstance();
    Tagged<PyObject> initial = current_frame_->localsplus()->Get(op_arg);
    cell->set_value(initial);
    current_frame_->localsplus()->Set(op_arg, cell);
  })

  // 加载cell对象到栈上（不解引用），
  // 该字节码会在生成持有对外部函数变量引用的内部函数时出现。
  // 例子：
  // LOAD_CLOSURE             1 (x)
  // BUILD_TUPLE              1
  // LOAD_CONST               2 (<code object say, file "example.py">)
  // MAKE_FUNCTION            8 (closure)
  // STORE_FAST               0 (say)
  INTERPRETER_HANDLER_DISPATCH(LoadClosure, {
    Tagged<PyObject> cell = current_frame_->localsplus()->Get(op_arg);
    assert(IsCell(cell));
    PUSH(cell);
  })

  // 加载cell对象指向的实际值对象
  INTERPRETER_HANDLER_WITH_SCOPE(LoadDeref, {
    Tagged<Cell> cell =
        Tagged<Cell>::cast(current_frame_->localsplus()->Get(op_arg));
    assert(IsCell(cell));

    Tagged<PyObject> value = cell->value_tagged();
    if (!value.is_null()) [[likely]] {
      PUSH(value);
      break;
    }

    Tagged<PyCodeObject> code_object = current_frame_->code_object_tagged();
    Tagged<PyString> var_name = Tagged<PyString>::cast(
        code_object->localsplusnames()->GetTagged(op_arg));
    char kind = code_object->localspluskinds()->Get(op_arg);
    if (kind & PyCodeObject::LocalsPlusKind::kFastFree) {
      Runtime_ThrowErrorf(ExceptionType::kNameError,
                          "cannot access free variable '%s' where it is "
                          "not associated with a value in enclosing scope",
                          var_name->buffer());
    } else {
      Runtime_ThrowErrorf(ExceptionType::kNameError,
                          "local variable '%s' referenced before assignment",
                          var_name->buffer());
    }
    goto pending_exception_unwind;
  })

  // 修改cell对象所指向的实际值对象
  INTERPRETER_HANDLER_DISPATCH(StoreDeref, {
    Tagged<PyObject> value = POP_TAGGED();
    Tagged<Cell> cell =
        Tagged<Cell>::cast(current_frame_->localsplus()->Get(op_arg));
    assert(IsCell(cell));
    cell->set_value(value);
  })

  INTERPRETER_HANDLER_DISPATCH(JumpBackward, {
    current_frame_->set_pc(current_frame_->pc() - (op_arg << 1));
  })

  // 一般作为持有外部函数闭包变量的内部函数的第一条字节码出现
  // 在这个字节码中，我们将外部函数的闭包变量列表（cellvars，本质上是一系列Cell对象）
  // 注入进被调函数栈帧的自由变量列表（freevars）当中，
  // 这样在被调函数中就可以通过LOAD_DEREF/STORE_DEREF字节码来读写这些闭包变量了。
  INTERPRETER_HANDLER_DISPATCH(CopyFreeVars, {
    Tagged<PyCodeObject> code_object = current_frame_->code_object_tagged();
    assert(op_arg == code_object->nfreevars());

    Tagged<PyFunction> func_of_current_frame = current_frame_->func_tagged();
    assert(IsPyFunction(func_of_current_frame));

    Tagged<PyTuple> func_closures = func_of_current_frame->closures_tagged();
    int offset = code_object->nlocalsplus() - op_arg;
    for (auto i = 0; i < op_arg; ++i) {
      Tagged<PyObject> object = func_closures->GetTagged(i);
      assert(IsCell(object));
      current_frame_->localsplus()->Set(i + offset, object);
    }
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
    GOTO_ON_EXCEPTION(Runtime_ExtendListByItratableObject(
        Handle<PyList>::cast(list), source));
  })

  // 当使用*或**尝试展开iterable object或dict时，
  // 参数的个数在运行前是不确定的，因此必须走CallFunctionEx字节码由解释器手工进行展开!
  //
  // 【例子1】
  // 代码：f(*[1, 2])
  // 字节码：
  //   LOAD_NAME (f)
  //   BUILD_LIST (1, 2)      # 运行时构造列表（或从变量加载）
  //   CALL_FUNCTION_EX 0     # 0 表示没有关键字参数字典
  //
  // 【例子2】
  // 代码：f(a=1, **d)
  // 字节码：
  //   ... 加载 f ...
  //   BUILD_MAP (a:1)        # 先把普通关键字打包成 dict
  //   LOAD_NAME (d)          # 加载动态字典
  //   DICT_MERGE 1           # 关键：将 d 合并到前面的 dict 中
  //   CALL_FUNCTION_EX 1     # 1 表示栈顶存在关键字参数字典
  //
  // 【例子3】
  // 如果参数太多，逐个执行 PUSH
  // 指令会占用过多的字节码空间和虚拟机栈深度。此时编译器可能会选择将参数预先打
  // 包成元组，通过CALL_FUNCTION_EX 一次性传入。
  // 代码：f(1, 2, ..., 31)  # 超过 30 个参数
  // 字节码：
  //   LOAD_NAME (f)
  //   LOAD_CONST ((1, 2, ..., 31)) # 编译器直接把参数打包成一个常量元组
  //   CALL_FUNCTION_EX 0           # 降级使用 CALL_FUNCTION_EX
  INTERPRETER_HANDLER_WITH_SCOPE(CallFunctionEx, {
    Handle<PyDict> kw_args;
    // `op_arg & 1`表示栈顶存在关键字参数字典
    if ((op_arg & 1) != 0) {
      Handle<PyObject> kw = POP();
      if (!IsPyDict(kw)) [[unlikely]] {
        Runtime_ThrowError(ExceptionType::kTypeError,
                           "CALL_FUNCTION_EX expected a dict for **kwargs");
        goto pending_exception_unwind;
      }
      kw_args = Handle<PyDict>::cast(kw);
    }

    Handle<PyObject> args_obj = POP();
    Handle<PyObject> callable;
    Handle<PyObject> host;
    PopCallTarget(callable, host);

    Handle<PyTuple> pos_args;
    if (!args_obj.is_null()) {
      if (IsPyTuple(args_obj)) {
        pos_args = Handle<PyTuple>::cast(args_obj);
      } else {
        ASSIGN_GOTO_ON_EXCEPTION(pos_args,
                                 Runtime_UnpackIterableObjectToTuple(args_obj));
      }
    }

    InvokeCallableWithNormalizedArgs(callable, host, pos_args, kw_args);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(Call, {
    int arg_count = op_arg;

    Handle<PyTuple> args;
    if (arg_count > 0) {
      args = PyTuple::NewInstance(arg_count);
      while (arg_count-- > 0) {
        args->SetInternal(arg_count, POP());
      }
    }

    Handle<PyObject> callable;
    Handle<PyObject> host;
    PopCallTarget(callable, host);

    InvokeCallable(callable, host, args, ReleaseKwArgKeys());
  })

  INTERPRETER_HANDLER_DISPATCH(
      KwNames, { kwarg_keys_ = current_frame_->consts()->GetTagged(op_arg); })

  INTERPRETER_HANDLER_WITH_SCOPE(CallIntrinsic1, {
    Handle<PyObject> arg = POP();
    switch (op_arg) {
      case UnaryIntrinsic::kListToTuple: {
        Handle<PyTuple> tuple_result;
        ASSIGN_GOTO_ON_EXCEPTION(tuple_result,
                                 Runtime_IntrinsicListToTuple(arg));
        PUSH(tuple_result);
        break;
      }
      case UnaryIntrinsic::kImportStar: {
        GOTO_ON_EXCEPTION(
            Runtime_IntrinsicImportStar(arg, current_frame_->locals()));
        PUSH(handle(isolate_->py_none_object()));
        break;
      }
      default:
        Runtime_ThrowErrorf(ExceptionType::kRuntimeError,
                            "unsupported intrinsic for CALL_INTRINSIC_1: %d",
                            op_arg);
        goto pending_exception_unwind;
    }
  })

#undef GOTO_ON_EXCEPTION
#undef ASSIGN_GOTO_ON_EXCEPTION
#undef INTERPRETER_HANDLER_NOOP
#undef INTERPRETER_HANDLER_WITH_SCOPE
#undef INTERPRETER_HANDLER_DISPATCH
#undef INTERPRETER_HANDLER

pending_exception_unwind: {
  // 异常展开（zero-cost exception table）：
  // 1) 以 pending_exception_pc_ 作为查表位置，查询当前 frame 是否存在 handler；
  // 2) 命中：恢复 value stack 深度；若 handler 要求 push-lasti，则把 origin
  // ip(code units)
  //    压栈；随后把异常对象压栈并跳转到 handler；
  // 3) 未命中：销毁当前 frame，继续向 caller 传播。
  if (current_frame_ == nullptr) [[unlikely]] {
    goto exit_interpreter;
  }

  auto* exception_state = isolate_->exception_state();

  // 注意：这里必须强制更新 pending_exception_pc；
  // 不能加 if (pending_exception_origin_pc == kInvalidProgramCounter)
  // 前置判断！
  // 因为 pending_exception_pc 表示的是当前异常传播到哪条字节码了，
  // 既然异常传播到了当前字节码，就必须对其进行更新。
  // 否则会出现无法触发当前字节码对应的 exception handler的 bug。
  exception_state->set_pending_exception_pc(current_frame_->pc() -
                                            kBytecodeSizeInBytes);

  if (exception_state->pending_exception_origin_pc() ==
      ExceptionState::kInvalidProgramCounter) {
    exception_state->set_pending_exception_origin_pc(
        exception_state->pending_exception_pc());
  }

  // 如果在当前栈帧中成功找到有效的exception handler，那么跳转过去处理错误
  ExceptionHandlerInfo handler_info;
  if (ExceptionTable::LookupHandler(current_frame_->code_object(),
                                    exception_state->pending_exception_pc(),
                                    handler_info)) {
    // 清理操作数栈上多余的元素，为执行exception handler做好准备
    current_frame_->set_stack_top(handler_info.stack_depth);

    if (handler_info.need_push_lasti) {
      // handler中的第一条Reraise字节码会消费lasti
      PUSH(PySmi::FromInt(exception_state->pending_exception_origin_pc()));
    }
    // handler中的第一条PushExcInfo或Reraise字节码会消费exception对象
    PUSH(exception_state->pending_exception_tagged());
    //  handler中的第一条PushExcInfo字节码会消费exception origin pc
    stack_exception_origin_pc_ = exception_state->pending_exception_origin_pc();

    // exception已经被提交给handler处理，撤销虚拟机中存在
    // 待处理的pending exception的状态。
    exception_state->Clear();

    // 跳转到目标handler
    current_frame_->set_pc(handler_info.handler_pc);
    DISPATCH();
  }

  // 否则继续向上回溯函数调用栈
  if (current_frame_->IsFirstFrame() || current_frame_->is_entry_frame()) {
    goto exit_interpreter;
  }
  UnwindCurrentFrameForException();
  DISPATCH();
}

#undef DISPATCH

unknown_bytecode:
  // 未知字节码，不抛错误，直接让虚拟机崩溃！
  std::fprintf(stderr, "unknown bytecode %d, arguments %d", op_code, op_arg);
  std::exit(1);

exit_interpreter:
  return;
}

// Python3.11+中引入的双槽位调用协议规定了两种栈布局:
// - 普通函数调用：栈底->[..., NULL, callable, arg1, ...]<-栈顶
// - 对象方法调用：栈底->[..., callable, self, arg1, ...]<-栈顶
//
// 这里需要根据arg1左边倒数第2个槽位是否为NULL，来确定属于哪一种调用操作，
// 并相应地完成从栈中提取数据的操作。
void Interpreter::PopCallTarget(Handle<PyObject>& callable,
                                Handle<PyObject>& host) {
  Handle<PyObject> callable_or_self = POP();
  Handle<PyObject> method = POP();

  host = Handle<PyObject>::null();
  callable = callable_or_self;
  if (!method.is_null()) {
    host = callable_or_self;
    callable = method;
  }
}

void Interpreter::InvokeCallable(Handle<PyObject> callable,
                                 Handle<PyObject> host,
                                 Handle<PyTuple> actual_args,
                                 Handle<PyTuple> kwarg_keys) {
  NormalizeCallable(callable, host);

  // Fast Path：如果是普通的python函数，那么直接创建并进入新的解释器栈帧
  if (IsNormalPyFunction(callable)) {
    FrameObject* frame;
    ASSIGN_RETURN_ON_EXCEPTION_VOID(
        isolate_, frame,
        FrameObjectBuilder::BuildFastPath(Handle<PyFunction>::cast(callable),
                                          host, actual_args, kwarg_keys));

    EnterFrame(frame);
    return;
  }

  // Slow Path：先对用户传入的实参进行归一化，再尝试调用callable的call虚方法
  Handle<PyTuple> pos_args;
  Handle<PyDict> kw_args;
  NormalizeArguments(actual_args, kwarg_keys, pos_args, kw_args);

  // 如果是NativeFunction，直接执行调用，不走开销昂贵的虚函数
  if (IsNativePyFunction(callable)) {
    auto func_object = Handle<PyFunction>::cast(callable);
    auto* native_func_ptr = func_object->native_func();

    Handle<PyObject> result;
    if (!native_func_ptr(host, pos_args, kw_args).ToHandle(&result)) {
      return;
    }

    PUSH(result);
    return;
  }

  // 兜底：调用对象的__call__虚函数
  Handle<PyObject> result;
  if (!PyObject::Call(callable, host, pos_args, kw_args).ToHandle(&result)) {
    return;
  }
  PUSH(result);
}

void Interpreter::InvokeCallableWithNormalizedArgs(Handle<PyObject> callable,
                                                   Handle<PyObject> host,
                                                   Handle<PyTuple> pos_args,
                                                   Handle<PyDict> kw_args) {
  NormalizeCallable(callable, host);

  if (IsNormalPyFunction(callable)) {
    FrameObject* frame;
    ASSIGN_RETURN_ON_EXCEPTION_VOID(
        isolate_, frame,
        FrameObjectBuilder::BuildSlowPath(Handle<PyFunction>::cast(callable),
                                          host, pos_args, kw_args));

    EnterFrame(frame);
    return;
  }

  Handle<PyObject> result;
  if (!PyObject::Call(callable, host, pos_args, kw_args).ToHandle(&result)) {
    return;
  }
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

void Interpreter::UnwindCurrentFrameForException() {
  // 当前栈帧已经发生错误，因此强制回溯到上一层栈帧
  DestroyCurrentFrame();
  ret_value_ = Tagged<PyObject>::null();

  isolate_->exception_state()->set_pending_exception_pc(
      ExceptionState::kInvalidProgramCounter);
  // 重要：将 pending_exception_pc 回溯为上一层栈帧的函数调用点地址
  if (current_frame_ != nullptr) {
    isolate_->exception_state()->set_pending_exception_pc(current_frame_->pc() -
                                                          kBytecodeSizeInBytes);
  }
}

}  // namespace saauso::internal
