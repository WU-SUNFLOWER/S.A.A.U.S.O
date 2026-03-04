# Interpreter::Run 异常传播治理计划

## 结论（先回答问题）

* 现状不合理：`Interpreter::Run` 作为 embedder 入口却返回 `void`，把“执行成功”和“带 pending exception 返回”混在同一类型里，调用方只能靠约定额外检查 `HasPendingException()`，这会导致 API 易误用与异常被静默吞噬。

* 从长期演进看，应将“可能失败”编码进函数签名，形成编译期约束，避免新调用点遗漏异常检查。

## 治理目标

* 将 `Interpreter::Run` 升级为显式失败语义的接口，使调用方必须处理失败分支。

* 与现有异常传播体系保持一致（`Maybe<T>` / `kNullMaybe` / pending exception 挂在 `Isolate`）。

* 保持异常对象由运行时统一格式化，不在 `Run` 内部做 I/O 输出。

## 方案选型

* 采用主方案：`void Run(...)` -> `Maybe<void> Run(...)`。

* 选择理由：

  * 与现有 `CallPython`、`ASSIGN_RETURN_ON_EXCEPTION*`、`RETURN_ON_EXCEPTION*` 语义一致；

  * `[[nodiscard]] Maybe<void>` 能在调用点形成更强约束；

  * 失败时仅返回 `kNullMaybe`，不破坏当前 pending exception 生命周期。

## 实施步骤（按顺序）

1. **修改接口签名与实现**

   * 文件：`src/interpreter/interpreter.h`、`src/interpreter/interpreter.cc`

   * 改动：

     * 声明改为 `Maybe<void> Run(Handle<PyFunction> boilerplate);`

     * 定义中把 `ASSIGN_RETURN_ON_EXCEPTION_VOID` 改为返回 `kNullMaybe` 的宏路径（`ASSIGN_RETURN_ON_EXCEPTION` ）。

     * `EvalCurrentFrame(); DestroyCurrentFrame();` 后返回 `JustVoid()`。

2. **收敛并修复所有调用点**

   * 文件：`src/main.cc`、`test/unittests/test-helpers.cc`（及检索出的其他调用点）

   * 改动：

     * 所有 `interpreter()->Run(...)` 改为检查 `IsNothing()`。

     * 在入口调用点，失败后保留 pending exception，交由既有格式化流程处理。

     * 清理“只靠 `HasPendingException()` 兜底”的弱约束路径，改成“先看返回值，再处理异常文本”的强约束路径。

3. **补充/调整单测覆盖**

   * 目标：

     * 覆盖 `Run` 成功返回 `Just`；

     * 覆盖 `Run` 失败返回 `Nothing` 且 pending exception 存在；

     * 覆盖辅助测试工具（`RunScript`/`RunScriptExpectExceptionContains`）在新契约下行为稳定。

   * 文件优先：`test/unittests/interpreter/*` 与 `test/unittests/test-helpers.cc`。

4. **构建与回归验证**

   * 至少执行：

     * 编译 `ut` 目标，确保签名改动无遗漏调用点；

     * 跑解释器相关测试（异常、exec、smoke、ops）；

     * 若主程序可构建，验证入口在异常脚本下返回非零并输出错误文本。

## 验收标准

* 不存在任何未处理的 `Run` 返回值调用点。

* `Run` 失败时，调用方可通过返回值立即判定失败，且 `Isolate` 内仍保留 pending exception 供格式化。

* 解释器相关单测全绿，且无新增编译告警/错误。

## 风险与兼容策略

* 风险：签名变更会导致外部 embedder 编译期不兼容。

* 策略：

  * 本次优先做“强约束治理”（直接改签名并全量修复仓内调用点）；

  * 如需平滑迁移，可后续加临时包装接口（例如 `RunLegacy`）并标记弃用，但不作为本次主路径。

