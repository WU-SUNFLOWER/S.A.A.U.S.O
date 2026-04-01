# 字节码解释器（Interpreter）

本文档收口 S.A.A.U.S.O 字节码解释器（`src/interpreter`）的架构要点、关键数据结构与 CPython3.12 的 call-shape 双槽位调用协议说明。

## 1. 解释器架构（`src/interpreter`）

- **Interpreter**：字节码执行入口与跨语言调用入口；负责维护当前栈帧链、异常展开状态与 computed-goto 的 dispatch table。`builtins` 字典本身归属 `Isolate`，Interpreter 按需读取。
- **builtins 字典（行为对齐用）**：
  - builtins 字典由 `BuiltinBootstrapper` 统一构建并挂到 `Isolate`（避免 Interpreter 构造函数膨胀），见 [builtin-bootstrapper.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.h)。
- 同时会注入最小异常类型对象体系：`BaseException/Exception` 以及 `TypeError/RuntimeError/ValueError/IndexError/KeyError/NameError/AttributeError/ZeroDivisionError/ImportError/ModuleNotFoundError/StopIteration` 等。
- **computed-goto dispatcher**：`Interpreter::EvalCurrentFrame()` 使用 256 槽 dispatch table（未知 opcode 默认跳到 `unknown_bytecode`），每个 handler 内优先创建 `HandleScope`，避免 GC 移动导致悬垂引用。
- **FrameObject（栈帧）**：
  - 保存 `stack/fast_locals/locals/globals/consts/names/code_object` 等字段，并在 `Iterate(ObjectVisitor*)` 中暴露为 GC roots。
  - **参数绑定与默认值**：函数调用的形参绑定主要在 `FrameObjectBuilder::BuildSlowPath/BuildFastPath` 中完成；支持位置参数、关键字参数、默认参数回填，以及 `*args/**kwargs` 的打包与注入。
- **异常表（exception table）**：
  - 解释器侧引入 `ExceptionTable` 来解析并线性扫描 `co_exceptiontable`（CPython 3.11+ 的 varint 编码格式），用于查找 try/except 的 handler，见 [exception-table.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/exception-table.cc)。
  - `PyCodeObject` 保存 `exception_table_` 并在 GC iterate 中显式遍历，`.pyc` 解析器也会读取该字段，见 [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h) 与 [cpython312-pyc-file-parser.cc](file:///e:/MyProject/S.A.A.U.S.O/src/code/cpython312-pyc-file-parser.cc)。
  - 当前解释器已实现 `pending_exception_unwind` 主路径：命中 handler 时恢复栈深并跳转，未命中时跨 frame 回溯。
- **调用约定（面向实现而非语义保证）**：
  - `CALL + KW_NAMES`：按 CPython3.12 “双槽位调用协议”组织 operand stack，`KW_NAMES` 提供 `kwarg_keys_`；`CALL_FUNCTION_EX` 处理 `f(*args, **kwargs)`；`DICT_MERGE` 处理 kwargs dict 合并。
  - `CALL_INTRINSIC_1`：当前至少支持 `INTRINSIC_LIST_TO_TUPLE` 与 `INTRINSIC_IMPORT_STAR`；新增 intrinsic 时需同步检查 dispatch、错误传播与解释器回归测试。

## 2. CPython3.12 Call 双槽位调用协议（关键）

CPython 3.12 为了优化“方法调用（obj.meth(...)）”的热点路径，引入了一个在字节码层面可表达的约定：把“是否需要隐式注入 self”编码到 operand stack 的两个前缀槽位中，从而让 `CALL` 在运行时只做非常轻量的栈调整。

### 2.1 核心栈布局

对任意一次调用，在执行 `CALL oparg` 前，operand stack 的尾部应满足：

```
..., method_or_NULL, callable_or_self, arg1, arg2, ..., argN
```

- `N == oparg`
- `method_or_NULL == NULL`：表示“普通调用”，此时 `callable_or_self` 就是被调用对象（callable）
- `method_or_NULL != NULL`：表示“方法调用形态”，此时
  - `callable_or_self` 实际存放的是 `self`
  - `method_or_NULL` 才是真正要调用的函数对象（裸 function / 裸 native function）

随后 `CALL` 的语义等价于（伪代码）：

```
args = stack_pointer - oparg
callable = stack_pointer[-(1 + oparg)]
method = stack_pointer[-(2 + oparg)]
if (method != NULL) {
  callable = method
  args--          // 把 self 纳入 args 视图
  total_args++
}
```

这套协议的关键在于：call-shape 的 `LOAD_ATTR` 可以直接把 `(method,self)` 放到栈上，避免创建中间的 `PyMethodObject`。

### 2.2 生产/消费这两个前缀槽位的指令

- 生产端（把双槽位布局准备好）：
  - `PUSH_NULL`：压入一个 NULL 哨兵
  - `LOAD_GLOBAL`：当 `oparg & 1 != 0` 时，先压 NULL 再压 value
  - `LOAD_ATTR`：当 `oparg & 1 != 0` 时，压入 `(method_or_NULL, self_or_value)` 的双槽位形态
- 消费端（按双槽位布局调用）：
  - `CALL`：消费 `method_or_NULL`/`callable_or_self` 两槽位 + `oparg` 个参数
  - `CALL_FUNCTION_EX`：同样消费两槽位，再消费 `args_obj` 与可选 `**kwargs`
- `KW_NAMES`：不改栈布局；它只是把关键字参数 key tuple 暂存起来供下一条 `CALL` 消费。

### 2.3 S.A.A.U.S.O 的实现索引（以仓库现状为准）

- 栈布局与字节码 handler：
  - `src/interpreter/interpreter-dispatcher.cc`
    - `PushNull`：真实压入 `Tagged<PyObject>::null()`
    - `LoadGlobal`：实现 `op_arg & 1` 的 NULL 前缀压栈
    - `LoadAttr`：当 `op_arg & 1`（call-shape）时走 `PyObject::GetAttrForCall`，直接压栈 `(method_or_NULL, self_or_value)`，不创建 `MethodObject`
    - `Call` / `CallFunctionEx`：消费双槽位；若 `method != NULL`，则直接以 `callable=method`、`host=self` 进入调用路径（不会重建 `MethodObject`）
    - `KwNames`：设置 `kwarg_keys_`，由 `ReleaseKwArgKeys()` 做“一次性消费并清空”
- “不分配 MethodObject 的方法查找”：
  - `src/objects/py-object.{h,cc}`：`PyObject::GetAttrForCall(self, name, self_or_null)` 返回“可调用对象”，并通过 out-param 返回是否需要注入的 `self`
  - `src/objects/py-object-klass.{h,cc}`：`PyObjectKlass::Generic_GetAttrForCall` 在命中 `PyFunction/PyNativeFunction` 时直接返回裸函数 + `self_or_null=self`；命中实例 dict 时不绑定；其它情况回退到普通 `GetAttr` 语义
- host 注入与调用路径（与双槽位协议的衔接点）：
  - `src/interpreter/interpreter.{h,cc}` / `src/interpreter/interpreter-dispatcher.cc`
    - 调用入口统一走 `Interpreter::InvokeCallable*`，其核心是把 `host` 传给 `FrameObjectBuilder::PrepareForFunction`/`BuildFastPath`/`BuildSlowPath`
    - `FrameObjectBuilder` 若 `host` 非空，会将其写入 `localsplus[0]` 作为隐式 `self`

### 2.4 迁移/扩展注意事项

- 如果未来引入更完整的 descriptor/`__get__` 协议，需要同步扩展 `GetAttrForCall` 的“是否可拆成 (method,self)”判定规则；目前仅对 `PyFunction/PyNativeFunction` 做了特判优化。

