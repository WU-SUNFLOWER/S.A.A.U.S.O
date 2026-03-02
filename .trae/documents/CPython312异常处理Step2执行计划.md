# CPython 3.12 字节码（MVP）异常/错误处理机制：Step 2 执行计划（C++ 抛错 API + 逐步替换 fprintf+exit）

## 目标

* 支持 VM 内部 C++ 代码“抛出 Python 异常并向上传播”，与现有 Step 1 的 `try/except/finally` 语义闭环打通。

* 设计并落地一套 V8-like 的内部抛错 API，使 builtins/runtime 等模块能用统一协议替换 `std::fprintf + std::exit`。

* 先小范围试点替换（builtins/runtime 的热点路径），验证稳定后再铺开替换范围。

## 非目标（MVP 明确不做）

* 不引入 C++ 语言级异常（`throw/catch`），仅使用显式的 error indicator + 失败返回协议。

* 不追求 CPython 级别完整 traceback / exception chaining / `BaseException.args` 等细节语义一次到位；优先保证“可抛出、可捕获、可跨栈帧传播、不会直接退出进程”。

* 不在本阶段一次性替换全仓库所有 `fprintf+exit`，而是建立机制并形成可持续迁移路径。

## 当前基线（以仓库现状为准）

* Step 1 已具备：

  * 传播态异常：`Interpreter::pending_exception_` + `pending_exception_unwind`（exception table 查表与跨帧展开）。

  * 捕获态异常：`caught_exception_` + `PUSH_EXC_INFO/POP_EXCEPT`（供无参 `raise` / nested handler）。

  * 单测：`test/unittests/interpreter/interpreter-exceptions-unittest.cc` 覆盖 try/except/finally 与跨帧 unwind。

* 仍缺少（阻碍 Step 2）：

  * C++ 侧统一“抛异常/创建异常对象/传播失败”的 API 与工程化约定（当前大量 `fprintf+exit` 直接终止进程）。

  * builtins/runtime 中的错误路径无法被 Python `try/except` 捕获（因为进程退出）。

  * 顶层“未捕获异常”的归宿未工程化：当前 `RunScript/Run` 不会显式报告/失败（需要在 embedder/测试框架层做显式处理）。

## 设计约束与核心原则

* 异常状态唯一来源：以解释器的 pending exception（传播态）为“error indicator”，并允许 C++ 内部设置它。

* 失败返回协议必须显式且低侵入：目前 S.A.A.U.S.O 中已经实现类似 V8 风格的 `MaybeHandle<T>`。

* 不让“未捕获异常”静默吞掉：embedder（`main`/单测框架）必须能探测并处理 pending exception。

## 方案（V8-like 风格落地）

### 1) 异常创建（Exception Factory）

* 新增一个集中式异常构造入口（建议位置：`src/interpreter/` 或 `src/runtime/`，以便 builtins/runtime 引用，且不强迫 `src/utils` 依赖上层）。

* 能力：

  * 从 `builtins` 里按名字拿到异常 type object（例如 `TypeError/ValueError/...`）。

  * 统一实例化异常对象（`ConstructInstance`），并预留“写入 message/args”的扩展点（MVP 可先不做或仅存简单字段）。

* 同步梳理异常类型注入：当前 string table 含 `ZeroDivisionError/StopIteration`，但 bootstrapper 未注册；若 Step 2 替换涉及这些类型，需要补齐注册。

### 2) 抛出异常（Throw API）

* 在 `Interpreter`（或 `Isolate`）上提供“设置 pending exception”的统一入口：

  * `Throw(Handle<PyObject> exception)`：设置传播态异常，保持 pc/origin 的默认策略（由下一次 `DISPATCH()` 触发 unwind 时补齐）。

  * 便捷封装：`ThrowTypeError(...) / ThrowValueError(...) / ThrowRuntimeError(...)` 等，内部用 Exception Factory 创建实例后调用 `Throw(...)`。

* 失败返回约定：

  * 对返回 `Handle<PyObject>` 的 builtins/runtime：发生错误时返回 `Handle<PyObject>::null()`，并保证 pending exception 已设置。

  * 对返回 `void` 的 helper：改为返回 `bool`（或 `Maybe<void>`），由调用方在必要时提前结束后续逻辑，避免继续操作栈/容器导致二次破坏。

### 3) C++ 侧捕获（TryCatch）

* 提供一个 RAII 风格的 `TryCatch`（语义类似 `v8::TryCatch`，但实现基于 pending exception）：

  * 构造时记录“进入前的 pending exception 状态”；

  * `HasCaught()` 检测当前是否有 pending exception；

  * `Exception()` 取出异常对象（并可选择清除 pending）；

  * `ReThrow()` 重新设置 pending exception 继续向上抛。

* 用途：

  * 测试框架/宿主（embedder）在 C++ 侧调用解释器/运行时 API 时，可以用 TryCatch 做“可控失败”而不是 `exit(1)`。

### 4) 顶层处理策略（避免静默失败）

* 单测框架：`BasicInterpreterTest::RunScript` 在运行后断言“无未捕获异常”；若存在，提取异常类型名并让用例失败（而不是退出进程）。

* 示例入口（`src/main.cc`）：运行后检测 pending exception；若存在则打印简化错误信息并返回非 0（保持 CLI 可用性）。

## 迁移策略（分阶段替换 fprintf+exit）

### 阶段 1：建立机制 + 少量试点替换（优先 builtins/runtime）

* 选择错误密集且测试容易覆盖的点位优先替换（建议第一批）：

  * `builtins-exec.cc`：参数/kwargs 校验错误 → `TypeError/RuntimeError`（可用 try/except 覆盖）。

  * `builtins-reflection.cc`（isinstance/build\_class 相关）→ `TypeError`。

  * `runtime-exec.cc`：globals/locals 校验、编译器开关禁用 → `TypeError/RuntimeError`。

* 为每个替换点补充端到端单测：用 Python `try/except` 捕获对应异常类型并 `print("ok")`，防止回归为 `exit(1)`。

### 阶段 2：扩展到更多核心路径

* 将迁移范围逐步扩大到：

  * `src/runtime/runtime-reflection.cc`（type()/magic invoke）；

  * `src/interpreter/frame-object-builder.cc`（调用参数绑定 TypeError）；

  * `src/objects/*` 中与用户语义强相关的 TypeError/AttributeError（需要评估依赖方向与最小侵入方式）。

### 阶段 3：建立统一规范与“禁止新增 exit”守门

* 形成约定：用户可触发的错误路径不得 `exit(1)`，必须通过 Throw API 传播；仅保留极少数“VM 内部一致性断言失败”的 fail-fast（可继续用 `assert` 或保留少量 `fprintf+exit`，但应明确边界）。

## 验证与回归

* 构建并运行 `ut`，重点关注：

  * `interpreter-exceptions-unittest.cc` 全通过；

  * 新增“builtins/runtime 抛错可捕获”的用例；

  * 现有非异常用例无行为回归（例如 import/exec 等路径）。

* 在 ASan 配置下跑一次异常相关用例（确保 unwind/stack 截断不会引入越界或悬垂 handle）。

