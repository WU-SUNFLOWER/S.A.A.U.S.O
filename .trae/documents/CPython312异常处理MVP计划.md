# CPython 3.12 字节码（MVP）异常/错误处理机制：第(0)(1)步调研结论 + 实施计划

## 结论：你的两步走思路是否合理

整体合理且可行，且拆分顺序是正确的：

* Step 1 先把“解释器层面的 try/except/finally 语义闭环”跑通，能够快速形成端到端回归用例，避免在 C++ 层大范围替换 `fprintf+exit` 时缺少保护网。

* Step 2 再把“VM 内部 C++ 报错路径”逐步迁移到统一的异常 API 上，最终达到“可捕获、可传播、可生成 traceback”的一致行为。

需要补充的一点是：即便 Step 1 暂不覆盖“C++ 主动抛异常”，也仍然必须引入最小异常状态与异常对象表示，否则 `RAISE_VARARGS`、`except T:` 匹配、`finally` 的 `RERAISE` 无法落地。

***

## (0) CPython 3.12 异常机制实现原理（MVP 视角）

### 0.1 两类异常状态：传播中的异常 vs 已捕获的异常信息

* 传播中的异常（error indicator）：更接近“当前执行路径失败了，异常正在向上传播”的状态。大量 C-API 约定为“返回 NULL/-1 表示失败 + error indicator 已设置”。

* 已捕获的异常信息（caught exception state）：`try/except` 捕获后，为了支持 `sys.exc_info()`、嵌套 try、re-raise 等，需要维护一份“异常信息栈”，并在进入/退出 except 时保存/恢复。

MVP 的关键点：SAAUSO 也需要同时具备这两类语义槽位，否则嵌套 `try/except/finally` 会出现“捕获态覆盖传播态/恢复错误”等问题。

### 0.2 3.11+ zero-cost exception handling：exception table 驱动

CPython 3.12 的 `try/except/finally/with` 不再依赖旧式 block stack 伪指令，而是：

* 编译期为 code object 构建 `co_exceptiontable`（异常表）

* 运行期仅在发生异常时查表，根据当前指令偏移定位 handler，并做：

  * 将 value stack 回退到表项指定的 stack-depth

  * 可选 push-lasti（用于 finally 末尾的 re-raise 纠正 traceback 指向）

  * 将异常对象压栈

  * 跳转到 handler target

规范级参考：CPython 内部文档 `InternalDocs/exception_handling.md`（它描述了表项语义与处理流程，适合直接对齐实现）。

### 0.3 与 try/except/finally 强相关的 opcode 家族（语义层面）

实现 MVP try/except/finally 通常绕不开：

* `RAISE_VARARGS`：显式 raise（含 raise-from）

* `PUSH_EXC_INFO`：进入 except handler 时保存/更新异常信息栈

* `CHECK_EXC_MATCH`：`except T:` 的类型匹配（含 tuple-of-types 与 subclass）

* `POP_EXCEPT`：except 结束时恢复异常信息

* `RERAISE`：finally 末尾/except 内 re-raise（常与 push-lasti 配套）

（`WITH_EXCEPT_START` 属于 with 语义，MVP 若暂不实现 with 可延后。）

***

## (1) SAAUSO 基建现状盘点：已有基础与缺口

### 1.1 已有基础（对 MVP 有价值）

* `.pyc` 解析器已读取 CPython 3.12 的 `co_exceptiontable` 并写入 `PyCodeObject::exception_table_`

  * 这意味着“异常表载体”已经具备，后续主要缺“解释器消费表并执行异常控制流”的实现。

### 1.2 关键缺口（阻塞 try/except/finally 的点）

按“是否直接阻塞语义闭环”排序：

1. 解释器侧缺少异常控制流 opcode 的定义与 handler（bytecode 子集内未包含异常相关指令）
2. `co_exceptiontable` 未被解释器执行循环解码/查表/应用（缺查表、栈回退、push-lasti、跳转）
3. 缺少运行时异常状态容器（pending exception + exc\_info\_stack + lasti 相关保存）
4. 缺少最小异常对象体系与类型匹配机制（至少能实现 `except T:`，并有 subclass 判断）
5. 大量路径仍是 `fprintf+exit`，缺少统一“抛出异常并传播”的协议与出口（即便实现 try/except，异常产生点若直接 exit 也无法捕获）

***

## 实施计划（后续进入执行阶段要做什么）

说明：本节是为了把第(2)子步骤（实现字节码 + 单测跑通）落到可执行的工程任务上；当你确认本计划后，再进入实际代码修改。

### 阶段 A：定义 MVP 异常状态与失败返回协议（先定“骨架”）

* 在解释器侧引入“异常状态容器”（单线程 MVP 可先挂在 Interpreter 上）：

  * pending exception（传播中异常）

  * exc\_info\_stack（捕获态异常信息栈）

  * reraise 所需的 lasti 保存/恢复辅助状态

* 选定一套对现有代码扰动最小的失败返回协议：

  * 引入仿V8风格的 `MaybeHandle<T>` 显式 ok/err

### 阶段 B：让解释器能够“抛出 → 查表 → 跳转到 handler → 匹配/清理 → 继续执行”

* 实现 exception table 的解码与查找：

  * 将 raising instruction 的偏移映射到 handler 表项

  * 应用 stack-depth 回退、push-lasti、push exception、设置新 IP

* 增加并实现异常相关 opcode handler：

  * `RAISE_VARARGS / PUSH_EXC_INFO / CHECK_EXC_MATCH / POP_EXCEPT / RERAISE`

* 最小异常对象体系（先覆盖常见内建异常）：

  * `BaseException` 及若干子类：`TypeError/NameError/AttributeError/IndexError/KeyError/ValueError/RuntimeError` 等

  * 类型匹配：instance-of + subclass-of（支持 `except (T1, T2):` 的 tuple-of-types）

### 阶段 C：单测优先（端到端回归）

* 增加解释器端到端单测覆盖：

  * `try/except`（命中/不命中、tuple-of-types、re-raise）

  * `try/finally`（正常路径与异常路径）

  * `try/except/finally` 组合

* 用这些用例作为“替换 `fprintf+exit` 为异常 API”时的回归保护网。

***

## 本轮（第(0)(1)步）交付物

* CPython 3.12 异常处理机制的 MVP 要点（zero-cost + exception table + opcode 族 + 两类异常状态）

***

## (2) Step 1 已实现但“无法正常运行”：排查与修复计划（进入执行阶段后做什么）

说明：你已经完成 Step 1 的代码编写，但目前无法正常运行。这里给出一个“按最短闭环优先”的排查与修复顺序，目标是尽快把 try/except/finally 的端到端单测跑通，并形成后续 Step 2 的回归保护网。

### 2.1 复现与定界（先把问题稳定下来）

* 只跑与异常相关的解释器端到端用例（优先 `interpreter-exceptions-unittest.cc`），记录：

  * 失败类型：编译失败 / 运行时崩溃 / 断言 / 用例输出不符合预期 / 卡死。

  * 失败用例对应的 Python 片段（`try/except`、`try/finally`、`raise`/`raise` 无参、`except (T1, T2)`）。

  * 失败时最后执行到的 opcode、pc、stack\_top、pending/caught 异常状态（必要时用最小诊断打印）。

* 若出现崩溃或越界，优先在 ASan/UBSan 构建下复现一次，以便直接定位到访问点。

### 2.2 优先修复“高概率、低成本、直接阻塞语义”的问题

* 修正 pending exception 的 pc 计算一致性：

  * 确认 `pending_exception_pc_` 统一表示“发生异常的那条指令”的 pc（与 exception table 的查表单位一致）。

  * 核对并修复所有写入 `pending_exception_pc_` 的路径，让它们与 `RAISE_VARARGS` 保持一致（避免出现 `pc()+1` 这类明显偏移错误导致 handler 查找必然 miss）。

* 统一 `push-lasti / origin_pc` 的单位与协议：

  * 明确 `pending_exception_*_pc_`、栈上 `lasti`、exception table 的 `start/size/target` 是否统一采用 byte offset（或统一采用 code units 并在边界处转换）。

  * 修正 `RERAISE` 从栈中恢复 lasti 时的单位假设，确保与 `pending_exception_unwind` 压栈的 lasti 一致。

* 强化 exception table 解码的健壮性（不改变语义前提下）：

  * 为 varint 解码与线性扫描增加边界检查，避免损坏/截断的表导致越界读，从而把问题从“崩溃”转成“查表 miss 并向上展开”。

### 2.3 针对 try/except/finally 的语义校验点（逐条对齐）

* 栈深回退：命中 handler 时必须把 operand stack 截断到 `stack_depth`，再按约定压入 `lasti`（可选）与异常对象。

* `PUSH_EXC_INFO/POP_EXCEPT`：进入 except 时保存/更新 caught 状态；离开 except 时恢复 caught 状态，避免嵌套 except 覆盖导致 `raise` 无参行为错误。

* `CHECK_EXC_MATCH`：覆盖三类输入：

  * 单个异常类型（type object）

  * tuple-of-types（其中元素必须是 type object）

  * 非法 match\_type（应抛 `TypeError`，并确保可被外层 try 捕获）

* `RAISE_VARARGS`：覆盖 0/1/2 三种形态，并确认“抛 type object → 自动实例化”与 “`raise` 无参 → 复抛 caught\_exception\_”的行为。

### 2.4 回归与扩展（确保修复可持续）

* 在现有异常用例基础上补齐“能直接卡住/跑偏”的边界单测：

  * nested try/except（两层 caught 状态保存/恢复）

  * `except (T1, T2)` 中含非法元素触发 TypeError 的可捕获性

  * `try/finally`：正常路径与异常路径都执行 finally

* 运行完整解释器相关用例集，确保异常机制没有破坏现有控制流、调用协议与栈平衡。

