## 目标

从“长期更推荐、更可持续、架构更健康、对齐 CPython 语义”的角度，完成 builtins dict 的归属治理：

* builtins 归属从 `Interpreter` 下沉到更语义化的“运行时实例状态”侧（当前工程内等价于 `Isolate`）。

* `Interpreter` 仅作为执行引擎消费 builtins，不再拥有/管理 builtins。

* `Execution::builtins()` 解除对 `Interpreter` 的结构性依赖，直接从 `Isolate` 获取 builtins。

* 初始化链路中，builtins 的引导与失败传播保持“可失败 + pending exception”约定一致，并清晰化失败时的可观测行为。

## 范围与非目标

* 范围：`builtins` 的所有权、访问入口、初始化时序、GC roots、以及相关调用点（runtime、tests、main）。

* 非目标：本轮不引入“多 interpreter / sub-interpreter”全套机制；不改造 public API 为 `Maybe<Isolate*>`（可作为后续第二阶段）。

## 设计决策

1. **builtins 归属**：builtins 属于 `Isolate`（VM 实例状态），而不是 `Interpreter`（执行器）。
2. **访问入口**：

   * 新增 `Isolate::builtins()` / `Isolate::builtins_tagged()` 访问器。

   * `Execution::builtins(Isolate*)` 改为直接返回 `isolate->builtins()`。
3. **初始化时序**：

   * 在 `Isolate::Init()` 中（`InitMetaArea()` 之后、创建 `Interpreter` 之前）完成 `BuiltinBootstrapper::CreateBuiltins()` 并写入 `isolate->builtins_`。

   * 若 `CreateBuiltins()` 返回空 `MaybeHandle`，要求 `Isolate` 已设置 pending exception；`Isolate::Init()` 停止继续初始化后续组件（Interpreter/ModuleManager）。
4. **失败语义**：

   * “初始化失败”通过 `isolate->HasPendingException()` 可观测；同时 `isolate->interpreter()` 可能为空。

   * `main` 与单测初始化基类需要对该失败形态做健壮处理（避免空指针、并尽可能打印 pending exception）。
5. **GC Roots**：

   * 由于 builtins 将不再被 `Interpreter::Iterate()` 访问，需要确认 `Isolate` 的 root 遍历入口能覆盖 `builtins_`（必要时补齐 root visitor 的访问）。

## 实施步骤（执行顺序）

### Step 0：基线与检视（只读）

* 枚举当前 builtins 的所有访问点（包含 tests、runtime、modules、builtins 本身）。

* 确认 GC roots 的遍历入口与覆盖范围：谁负责遍历 `Isolate` 的全局根集合。

* 确认 “pending exception + MaybeHandle 空值” 的约定在初始化阶段是否一致（断言/宏策略）。

交付物：

* 一份调用点清单（文件级别即可）。

* 一份 GC roots 覆盖确认结论（如需改动，列出改动点）。

### Step 1：把 builtins 字段下沉到 Isolate

* 在 `Isolate` 增加字段 `builtins_`（类型与 `Interpreter` 当前字段保持一致：`Tagged<PyObject>` 或更精确的 `Tagged<PyDict>`，按现有 Handle/Tagged 习惯决定）。

* 新增访问器：

  * `Tagged<PyDict> builtins_tagged() const`

  * `Handle<PyDict> builtins() const`

* 在 `TearDown()` 中清理 builtins 字段为 null（与 None/True/False 的清理一致）。

交付物：

* `Isolate` 持有 builtins 且提供访问器。

### Step 2：调整初始化链路，确保 builtins 在 Interpreter 之前可用

* 在 `Isolate::Init()` 中调用 `BuiltinBootstrapper::CreateBuiltins()`，成功后写入 `builtins_`。

* 仅在 builtins 初始化成功后才创建 `Interpreter` 与 `ModuleManager`。

* 确认 `BuiltinBootstrapper` 内不依赖 `Execution::builtins()` 的路径在 builtins 未就绪前不会形成循环依赖（若发现，改为使用局部 builtins handle 或引入 bootstrap 专用抛错路径）。

交付物：

* builtins 初始化时序稳定且失败可传播。

### Step 3：Interpreter 去 builtins 化（解除所有权）

* 从 `Interpreter` 中移除 `builtins_` 字段及其 `builtins()/builtins_tagged()` 访问器。

* 所有原本读取 builtins 的 Interpreter 内部逻辑（如果有）改为使用 `isolate_->builtins()`。

* 确认 `Interpreter::Iterate()` 不再需要访问 builtins（因为所有权已在 Isolate）。

交付物：

* `Interpreter` 变为纯执行器，不再拥有 builtins。

### Step 4：Execution 门面与 runtime 调用点治理

* 修改 `Execution::builtins(Isolate*)`：从 `isolate->interpreter()->builtins()` 改为 `isolate->builtins()`。

* 若存在其他通过 `interpreter()->builtins()` 间接访问的路径，一并改造为 `isolate->builtins()`。

交付物：

* runtime 对 builtins 的依赖不再穿透到 Interpreter。

### Step 5：单测与主程序健壮性治理

* 单测：把对 `interpreter()->builtins()` 的改写替换为 `isolate->builtins()`，保持语义不变。

* 主程序：统一处理 `Isolate` 初始化失败的路径（如果 `builtins` 初始化失败导致后续组件为空，应能打印 pending exception 并退出）。

交付物：

* 单测与 demo main 均不依赖 Interpreter 存在即可处理初始化失败。

### Step 6：GC roots 覆盖补齐与验证

* 若 GC root visitor 只遍历 `Interpreter::Iterate()`，需要增加对 `Isolate::builtins_` 的遍历。

* 验证路径：

  * Debug 构建通过。

  * 单测全量通过（`ut` 目标）。

  * 选择一个包含 `sysgc` 压力的用例，确保 builtins 存活与访问正确。

交付物：

* builtins 作为 root 被正确遍历，相关压力测试不回归。

## 风险与缓解

* 风险：初始化早期抛错依赖 builtins（例如 `Runtime_ThrowError` 需要 exception type）。

  * 缓解：bootstrapper 阶段尽量避免走 `Runtime_ThrowError`；必要时为初始化期提供不依赖 builtins 的 fail-fast 报错路径，或在 builtins 局部构造好最小异常类型后再抛。

* 风险：builtins 从 Interpreter 移出后 GC roots 不再覆盖。

  * 缓解：在 Step 0/6 明确确认并补齐 root visitor。

## 验收标准

* `Interpreter` 不再持有 builtins 字段与访问器，代码层面职责边界更清晰。

* `Execution::builtins()` 不依赖 `Interpreter` 结构，改为直接从 `Isolate` 获取。

* 初始化失败可通过 `HasPendingException()` 观测且不会产生空指针崩溃。

* Debug 构建与 `ut` 单测通过。

