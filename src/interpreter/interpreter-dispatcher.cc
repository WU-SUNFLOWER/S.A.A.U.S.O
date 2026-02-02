// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"
#include "src/handles/tagged.h"
#include "src/interpreter/bytecodes.h"
#include "src/interpreter/frame-object-builder.h"
#include "src/interpreter/frame-object.h"
#include "src/interpreter/interpreter.h"
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
#include "src/objects/py-string.h"
#include "src/objects/py-tuple.h"
#include "src/runtime/isolate.h"
#include "src/runtime/runtime.h"
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
    dispatch_table_initialized_ = true;
  }

  // 取指执行入口
  DISPATCH();

  INTERPRETER_HANDLER_NOOP(Cache)

  INTERPRETER_HANDLER_DISPATCH(PopTop, POP_TAGGED();)

  INTERPRETER_HANDLER_DISPATCH(PushNull, { PUSH(Tagged<PyObject>::null()); })

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

  INTERPRETER_HANDLER_DISPATCH(
      LoadBuildClass, { PUSH(builtins_tagged()->Get(ST_TAGGED(build_class))); })

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
      std::fprintf(stderr, "no locals found when storing %s\n",
                   Handle<PyString>::cast(key)->buffer());
      std::exit(1);
    }

    PyDict::Put(locals, key, POP());
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

  INTERPRETER_HANDLER_WITH_SCOPE(StoreAttr, {
    Handle<PyObject> attr_name = current_frame_->names()->Get(op_arg);
    Handle<PyObject> object = POP();
    Handle<PyObject> attr_value = POP();
    PyObject::SetAttr(object, attr_name, attr_value);
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
        std::fprintf(stderr,
                     "TypeError: got multiple values for keyword argument\n");
        std::exit(1);
      }

      PyDict::Put(target_dict, key, value);
      item = Handle<PyTuple>::cast(PyObject::Next(iter));
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
      Handle<PyObject> value =
          PyObject::GetAttrForCall(object, attr_name, self_or_null);

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

    std::fprintf(stderr, "NameError: name '%s' is not defined\n",
                 Handle<PyString>::cast(key)->buffer());
    std::exit(1);
  })

  INTERPRETER_HANDLER_WITH_SCOPE(ContainsOp, {
    Handle<PyObject> r = POP();
    Handle<PyObject> l = POP();
    bool result = PyObject::ContainsBool(r, l);
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
      case BinaryOpType::kFloorDivide:
      case BinaryOpType::kInplaceFloorDivide:
        PUSH(PyObject::FloorDiv(l, r));
        break;
      case BinaryOpType::kRemainder:
      case BinaryOpType::kInplaceRemainder:
        PUSH(PyObject::Mod(l, r));
        break;
    }
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
      std::fprintf(stderr,
                   "NameError: cannot access free variable '%s' where it is "
                   "not associated with a value in enclosing scope\n",
                   var_name->buffer());
    } else {
      std::fprintf(stderr,
                   "UnboundLocalError: local variable '%s' referenced before "
                   "assignment\n",
                   var_name->buffer());
    }
    std::exit(1);
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
    Runtime_ExtendListByItratableObject(Handle<PyList>::cast(list), source);
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
      if (!IsPyDict(*kw)) [[unlikely]] {
        std::fprintf(
            stderr,
            "TypeError: CALL_FUNCTION_EX expected a dict for **kwargs\n");
        std::exit(1);
      }
      kw_args = Handle<PyDict>::cast(kw);
    }

    Handle<PyObject> args_obj = POP();
    Handle<PyObject> callable;
    Handle<PyObject> host;
    PopCallTarget(callable, host);

    Handle<PyTuple> pos_args;
    if (!args_obj.is_null()) {
      pos_args = IsPyTuple(args_obj)
                     ? Handle<PyTuple>::cast(args_obj)
                     : Runtime_UnpackIterableObjectToTuple(args_obj);
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
        PUSH(Runtime_IntrinsicListToTuple(arg));
        break;
      }
      default:
        std::fprintf(stderr, "unsupported intrinsic for CALL_INTRINSIC_1: %d\n",
                     op_arg);
        std::exit(1);
    }
  })

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
    FrameObject* frame = FrameObjectBuilder::BuildFastPath(
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
                                                   Handle<PyObject> host,
                                                   Handle<PyTuple> pos_args,
                                                   Handle<PyDict> kw_args) {
  NormalizeCallable(callable, host);

  if (IsNormalPyFunction(callable)) {
    FrameObject* frame = FrameObjectBuilder::BuildSlowPath(
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
