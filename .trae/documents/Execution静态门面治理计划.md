# 架构治理计划：引入 execution::Execution（AllStatic 门面）收敛 Call API，并强化分层约束

## 0. 结论：你的建议是否合理？

总体合理，并且与 V8 的长期演进思路一致：把“执行入口（Call/Run）这种门面 API”收敛到 execution 层的静态工具类，避免把越来越多的“跨层转发接口”堆进 Isolate，进而导致 Isolate 职责膨胀。

对 SAAUSO 来说，这个方案尤其适合当前阶段：

* 我们刚完成了 **ExceptionState 下沉到 Isolate**，Isolate 已经承载了必要的全局执行状态（这类状态应当长期稳定）。

* “CallPython / builtins() 这类转发接口”属于 **执行器桥接门面**，更像 execution 层的一部分，而不是 Isolate 的核心状态接口。

* builtins/runtime 不应该直接依赖 interpreter（包括头文件与语义依赖）；引入 Execution 可以把这种依赖收敛为“execution → interpreter 的单向依赖”，让 runtime/builtins 永远只面对 execution/runtime API。

需要注意的边界条件：

* Execution 虽是静态类，但它并非“无依赖”：它仍要求调用者传入 `Isolate* isolate` 以获取当前隔离实例，并最终转发到当前的执行器（当前是 interpreter）。

* 静态类不等价于“永远不能支持多执行器”。只要 Execution 的实现内部再间接调用一个可替换的执行器入口（未来可演进为 executor 接口），静态门面与多执行器并不冲突。

## 1. 目标形态（治理后的分层与依赖方向）

### 1.1 分层职责

* **execution**

  * `ExceptionState`：Isolate 持有的传播态异常状态（error indicator）。

  * `Execution`（AllStatic）：提供执行门面 API（Call/内建访问），内部可转发到 interpreter 或未来其他执行器。

* **interpreter**

  * 实现 Python 字节码解释器与异常展开算法；只消费 `ExceptionState`。

* **runtime / builtins**

  * 语言语义与内建行为实现；允许调用 `Execution` 与 `Runtime_*`，禁止直接访问 interpreter。

### 1.2 依赖约束（关键）

* 允许：`runtime/builtins -> execution`、`execution -> interpreter`

* 禁止：`runtime/builtins -> interpreter`（include 与直接调用都算）

## 2. 方案设计（AllStatic Execution）

在 `src/execution` 新增：

* `execution.h / execution.cc`：

  * class Execution final : public AllStatic

  * API（按当前 SAAUSO 需要最小集）：

    * `static Handle<PyObject> Call(Isolate* isolate, Handle<PyObject> callable, Handle<PyObject> host, Handle<PyTuple> pos_args, Handle<PyDict> kw_args);`

    * `static Handle<PyObject> Call(Isolate* isolate, Handle<PyObject> callable, Handle<PyObject> host, Handle<PyTuple> pos_args, Handle<PyDict> kw_args, Handle<PyDict> bound_locals);`

    * `static Handle<PyDict> builtins(Isolate* isolate);`

  * 实现策略：

    * `execution.h` 只 include `handles.h`，前置声明 `PyObject/PyTuple/PyDict/Isolate`，不 include interpreter。

    * `execution.cc` include `isolate.h` 与 `interpreter.h`，实现内部做 `isolate->interpreter()->CallPython(...)` 的转发。

说明：

* 选择把 `builtins()` 也放进 Execution：它属于“执行环境门面”，且 runtime（例如 `Runtime_Execute*`）在注入 `__builtins__` 时确实需要它；把它放 Execution 能保持 runtime 不 include interpreter。

* 该门面本身不维护任何状态，符合你对“无需实例化”的语义判断。

## 3. 代码改造清单（治理落地）

### 3.1 从 Isolate 移除桥接 API（防膨胀）

* 从 `Isolate` 删除/弃用以下转发方法：

  * `Isolate::CallPython(...)`

  * `Isolate::builtins() const`

* 保留：`Isolate::exception_state()`（这是状态，不是门面）。

Execution 成为唯一对外“调用执行器”的门面入口。

### 3.2 runtime/builtins 全面改用 Execution（解除对 interpreter 的直接依赖）

目标：runtime/builtins 不再 include `src/interpreter/interpreter.h`，也不再调用 `Isolate::Current()->interpreter()->...`。

待改造点位（按现状 grep 命中为准）：

* `src/runtime/runtime-exec.cc`：用 `Execution::Call` 与 `Execution::builtins`

* `src/runtime/runtime-reflection.cc`：magic invoke 用 `Execution::Call`

* `src/runtime/runtime-py-string.cc`：`__str__` 调用用 `Execution::Call`

* `src/objects/klass-default-vtable.cc`：默认 print/call/getattr 等路径用 `Execution::Call`

* `src/builtins/builtins-py-list-methods.cc`：sort(key=) 路径用 `Execution::Call`

* `src/builtins/builtins-reflection.cc`：`__build_class__` builder 执行仍需调用；切换为 `Execution::Call`

### 3.3 强化分层约束的文档与“代码内入口收敛”

#### 文档（AGENTS.md）

在 [AGENTS.md](file:///e:/MyProject/S.A.A.U.S.O/AGENTS.md) 增加/强化一节 **Architecture Boundaries**：

* 分层图与依赖方向（允许/禁止）。

* 约定：

  * runtime/builtins 禁止 include `src/interpreter/interpreter.h`

  * 执行相关入口统一使用 `Execution::*`

  * 抛错统一使用 `runtime-exceptions`（已存在）与 `ExceptionState`

* 给出理由（面向人类与 AI 助手）：减少循环依赖、降低替换执行器成本、让 execution 层成为唯一“调用门面”。

#### 源代码（约束落地方式）

“强化约束”的最可靠手段是把调用面收敛到唯一入口：

* 删除 runtime/builtins 中所有 `Isolate::Current()->interpreter()` 的直接调用。

* 删除 runtime/builtins 中对 interpreter 头文件的 include。

这样后续新增代码如果想调用 interpreter，会在 review/编译阶段更显眼（需要主动 include interpreter，违反约定）。

（本轮不引入更激进的编译期分层检查宏/规则，避免影响现有 GN target 的可维护性；如后续需要，可单独提案增加 build-time lint。）

## 4. 迁移步骤（执行顺序）

1. 新增 `src/execution/execution.{h,cc}` 并加入 BUILD.gn
2. 将当前所有 `Isolate::CallPython/builtins` 调用点改为 `Execution::*`
3. 删除 `Isolate` 中对应桥接方法，并修复编译错误
4. 更新 runtime/builtins 头文件 include，确保不再 include interpreter
5. 更新 AGENTS.md 分层约束说明
6. 全量回归：`gn gen out/ut` + `ninja -C out/ut ut` + 运行 `out/ut/ut.exe`

## 5. 验收标准

* `src/execution/execution.*` 成为唯一 “Call/builtins 门面”入口，runtime/builtins 不再直接触达 interpreter。

* `Isolate` 不再承载 Call/builtins 转发 API，避免职责继续膨胀（仍保留 ExceptionState 等状态）。

* 全量 `ut` 通过，无行为回退。

