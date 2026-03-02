# SAAUSO（CPython 3.12 MVP）错误处理机制：第(0)步与第(1)步调研结论与缺口清单

## 背景与目标

当前 SAAUSO 在大量错误路径上仍以 `std::fprintf(stderr, ...) + std::exit(1)` 方式 fail-fast。你希望进入“兼容 CPython 3.12 字节码的 MVP 错误处理机制”阶段，优先跑通解释器 `try ... except ... finally` 的端到端单测，并逐步替换散落的退出式报错。

本计划文档仅覆盖你提出的：

* (0) 调研 CPython 3.12 的错误/异常处理机制实现原理

* (1) 调研 SAAUSO 当前基建，明确为 MVP 错误处理所缺失的基础设施与补齐方向

## (0) CPython 3.12 错误/异常处理机制：实现原理速览

### 0.1 两个不同但相关的“异常信息槽位”

* **未捕获、正在传播（error indicator）**\
  CPython 在 C-API 层面维护“每线程”的错误指示器（传统上是异常 type/value/traceback 的三元组）。大量 C-API 以 “返回 NULL/-1 表示失败 + error indicator 已被设置” 作为约定，调用方要么处理并清理异常，要么原样传播。

* **已捕获、用于** **`sys.exc_info()`（caught exception state）**\
  `try/except` 捕获后，异常会进入“已捕获”语义域（`sys.exc_info()`）。这与“正在传播”的 error indicator 不是一回事。CPython 需要在字节码层面维护“异常信息栈”，以支持嵌套 `try`、`except`、`finally` 与 re-raise。

对 SAAUSO 的意义：MVP 也需要同时具备“正在传播的 pending exception”以及“被捕获时的异常信息保存/恢复”能力，否则 `except`、`finally` 的语义会出现错乱。

### 0.2 3.11+ 的 zero-cost exception handling：exception table 驱动的控制流转移

CPython 3.11 起（3.12 继续沿用）对 `try/except/finally/with` 采用所谓 **zero-cost exception handling**：正常路径（无异常）几乎不支付额外开销；只有当异常发生时，才查表并跳转到 handler。其核心是：

* 编译阶段把 `SETUP_FINALLY`/`POP_BLOCK` 这类“伪指令”消去

* 为 code object 构建一份异常元数据表，存入 `co_exceptiontable`

* 运行时一旦发生异常，通过当前指令偏移在表中查找 handler，完成“栈深度回退 + 可选 lasti 入栈 + 异常入栈 + 跳转”

CPython 内部文档对该流程与表格式有一份非常清晰的描述（建议你把它当作“规范级”参考来实现 SAAUSO 的 MVP 行为）：\
【InternalDocs/exception\_handling.md】<https://github.com/python/cpython/blob/main/InternalDocs/exception_handling.md>

该文档给出的关键点包括：

* 异常表概念上是 5-tuple：`start, end, target, stack-depth, push-lasti`（都是 code units，不是 bytes）

* 处理异常时：

  1. 将 value stack 弹到 handler 指定深度
  2. 若 `push-lasti` 为真，把“触发异常的指令偏移”压栈
  3. 把异常对象压栈
  4. 跳转到 `target` 继续执行

* `RERAISE` 用于 `finally` 末尾的“带 lasti 的再次抛出”：当帧 IP 已在 finally 代码段内时，需要把 IP 修正为保存的 lasti，以便 traceback/语义指向真正的抛出处

### 0.3 与 try/except/finally 相关的关键字节码家族（语义层面）

在 3.12 里，异常控制流不再依赖旧式的 block stack 伪指令，而是依赖：

* **exception table**（决定“异常发生时跳去哪 + 需要把 value stack 回退到哪一层”）

* 一组 **异常相关 opcode**（管理异常信息栈、匹配、清理、re-raise 等）

你实现 MVP `try/except/finally` 时，通常绕不开这些语义能力（具体 opcode 名称以 CPython3.12 opcode 集合为准）：

* `RAISE_VARARGS`：raise / raise exc / raise exc from cause（显式 chaining）

* `PUSH_EXC_INFO`：进入 except handler 时保存/更新异常信息

* `CHECK_EXC_MATCH`：`except T:` 的类型匹配（包含 tuple-of-types 与 subclass 语义）

* `POP_EXCEPT`：except 块结束时恢复异常信息

* `RERAISE`：用于 finally 结尾或 except 内 re-raise（并配合 exception table 的 push-lasti 语义）

* （可选但常见）`WITH_EXCEPT_START`：with 语义需要（如果 MVP 暂不做 with，可先不实现）

## (1) SAAUSO 当前基建盘点：已有基础与明确缺口

### 1.1 与异常相关的“已有基础”（可复用）

* **`co_exceptiontable`** **已被解析并存入** **`PyCodeObject`**\
  `.pyc` 解析器已经把 CPython3.12 的 exception table 读入并写入 `PyCodeObject::exception_table_`：

  * 解析入口：[cpython312-pyc-file-parser.cc:L73-L107](file:///e:/MyProject/S.A.A.U.S.O/src/code/cpython312-pyc-file-parser.cc#L73-L107)

  * code object 结构体字段：[py-code-object.h:L34-L153](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h#L34-L153)

* **解释器主循环具备“unknown bytecode 直接退出”的兜底**\
  当前对于未实现的 opcode 会直接 `fprintf + exit(1)`：\
  [interpreter-dispatcher.cc:L726-L731](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc#L726-L731)

这些说明：SAAUSO 已经拿到了“3.12 风格异常表”的原始载体（bytes），但还缺少“解释异常表 + 执行异常控制流”的运行时语义。

### 1.2 当前最关键的缺口（为 MVP try/except/finally 必须补齐）

下面缺口按“能否直接阻塞 try/except/finally”排序。

#### A. 解释器侧缺少异常控制流的 opcode 实现

`src/interpreter/bytecodes.h` 当前枚举的 opcode 子集里，不包含异常相关指令：\
[bytecodes.h:L11-L64](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/bytecodes.h#L11-L64)

结果是：任何包含 `try/except/finally/raise` 的 3.12 字节码，都会落到 `unknown_bytecode` 并退出进程。

#### B. `co_exceptiontable` 未被解释器使用（缺“零成本异常表”的查表与跳转机制）

虽然 `PyCodeObject` 已保存 `exception_table_`，但解释器执行循环未解码/二分查找/应用表项。\
对齐 CPython 的 MVP 至少需要：

* 将 raising instruction offset 映射到 handler 表项

* 将 value stack 恢复到表项的 stack-depth

* 按表项决定是否 push lasti

* 把异常对象压栈并跳转到 handler

#### C. 缺“pending exception”与“异常信息栈”的运行时状态容器

当前 `FrameObject`/`Interpreter` 未见“异常状态槽位/栈”的设计，这会导致：

* 无法从 C++ helper/builtin/slot “抛出异常并回到解释器”

* 无法支持嵌套 try 的 `except`/`finally` 语义（保存/恢复 exc\_info）

* 无法正确实现 `RERAISE`（需要 lasti 与异常信息关联）

#### D. 缺异常对象体系与类型匹配机制（至少要支持 `except T:`）

仓库当前未发现 `BaseException`/traceback 对象/异常层级的实现与注入。\
要实现 `CHECK_EXC_MATCH`，至少需要：

* 异常“类型对象”的存在与子类关系判断

* 异常“实例对象”的最小表示（能携带 message/args 即可）

MVP 可以先只覆盖你当前 VM 已经在报错文案里出现频率最高的一批：`TypeError/NameError/AttributeError/IndexError/KeyError/ValueError/RuntimeError` 等，再逐步扩展。

#### E. 现有错误路径大量 `fprintf+exit`，缺“统一错误出口 + 传播协议”

当前各层大量存在 `std::fprintf(stderr, "...Error: ..."); std::exit(1);`，分布在 interpreter/objects/runtime/builtins/modules 等多个目录。\
这意味着：即便解释器支持 try/except，如果异常产生点仍然直接 exit，异常也无法被捕获。

### 1.3 MVP 基建补齐建议（实现阶段的设计导向，不在本轮执行）

这部分是为了让第(2)/(3)步在架构上更可控，避免“到处改返回值类型导致大范围扰动”。

* **建议引入统一的异常状态容器（类似 CPython 的 per-thread state）**\
  候选位置：`Interpreter`（单线程 MVP）
  容器至少包括：

  * `pending_exception`（正在传播的异常对象或三元组）

  * `exc_info_stack`（PUSH\_EXC\_INFO/POP\_EXCEPT 所需）

* **建议定义 VM 内部的“失败返回协议”**\
  目标是让大量 C++ helper/builtin/slot 能从 “exit” 改为 “设置 pending exception + 返回错误哨兵”，并由解释器统一接管展开与跳转。\
  需要提前选一种对现有代码扰动更小的方案，例如：

  * 引入 `MaybeHandle<T>` / `Result<T>`（显式携带 ok/err），逐步替换热点路径

  * 方案 2：沿用 `Handle<T>` 但约定“返回 null 表示异常”，并为“is\_try miss 返回 null”的场景补上更显式的 API（避免 null 语义冲突）

* **建议先做“解释器端到端”覆盖**\
  先写/跑通 `try/except/finally` 的解释器单测，确保“异常表 + opcode + 状态容器 + 抛出/捕获/再抛”整条链路可用，再逐步替换 C++ 层 `fprintf+exit`。

## 本轮交付物（你确认后进入实现阶段）

* CPython 3.12 异常处理机制的实现要点摘要（zero-cost exception handling、exception table、lasti/reraise、所需 opcode 族）

* SAAUSO 当前基建与缺口清单（已解析 co\_exceptiontable，但解释器未用；缺 opcode/异常状态/异常对象体系/统一错误出口）

* 后续实现阶段的“补齐方向”建议（状态容器位置、返回协议选型、测试优先级）

