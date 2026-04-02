# ValueRegistrySizeForTesting P0 治理计划

## Summary

本次改造目标是移除当前空壳测试接口 `Isolate::ValueRegistrySizeForTesting()`，并把受影响的 2 个 `embedder_ut` 用例改写为更真实的行为测试，避免继续依赖失真的“资源计数”断言。

成功标准：

* 删除 `include/` 与 `src/api/` 中的 `ValueRegistrySizeForTesting` 声明与实现。

* `test/embedder` 中不再出现 `ValueRegistrySizeForTesting()` 调用。

* 原 `HandleScope_Prevents_Memory_Leak` 重写为直接覆盖 public `EscapableHandleScope::Escape` / 生命周期语义的行为测试。

* 原 `Callback_EscapeScope_NoGrowth` 重写为“高频 callback 压力后行为仍正确”的行为测试。

* `embedder_ut` 在 debug 构建下通过；必要时补充 `all_ut` 编译/运行验证。

## Current State Analysis

### 1. 目标接口当前是空壳

* 公开声明位于 `include/saauso-isolate.h`：

  * `size_t ValueRegistrySizeForTesting() const;`

* 实现位于 `src/api/api-isolate.cc`：

  * 当前实现恒返回 `0`。

这意味着它没有连接到 VM 内部任何真实资源计数结构，无法提供有效测试信号。

### 2. embedder\_ut 受影响范围很小且明确

仓库内只发现 2 个 `embedder_ut` 用例使用该接口：

1. `test/embedder/embedder-script-unittest.cc`

   * `EmbedderPhase2Test.HandleScope_Prevents_Memory_Leak`

   * 当前做法：取 baseline，循环创建 `HandleScope + String::New`，再比较前后 registry size。

   * 由于接口恒返 `0`，断言退化为无意义的 `0 == 0`。

2. `test/embedder/embedder-interop-callback-unittest.cc`

   * `EmbedderPhase3Test.Callback_EscapeScope_NoGrowth`

   * 当前做法：高频调用脚本触发 callback，再比较前后 registry size。

   * 当前断言退化为无意义的 `0 <= 8`。

### 3. 当前更有价值的补位测试已经存在

* `test/embedder/embedder-script-unittest.cc` 中已有 `EmbedderGCTest.LocalSurvivesMovingGC`，
  它已经在验证 public `Local` 经 GC 后仍可安全读取，属于比“伪计数”更真实的句柄正确性测试。

### 4. 资源泄漏更适合 sanitizer 路径验收

* 文档 `docs/Saauso-Build-and-Test-Guide.md` 已明确支持 `is_asan=true` 构建路径。

* 因此“是否真正泄漏”应交给 ASAN/LSAN 路径，而非公开 API 暴露一个内部计数接口。

## Assumptions & Decisions

### 已确认决策

* 采用“删除空壳接口 + 改写受影响单测”为主路径，不尝试把该接口补成真实计数实现。

* 保持 embedder API 的公开边界，不新增新的公开计数型测试接口。

* 保留 `LocalSurvivesMovingGC` 等已有强行为测试，不做额外弱化或替换。

### 假设

* 当前仓库内不存在仓外 ABI 兼容性约束；本次变更以仓库内代码、样例、测试正确性为准。

* `embedder_ut` 是本次主要回归入口；如构建链路允许，可顺带检查 `all_ut` 入口的编译连通性。

## Proposed Changes

### 1. 删除空壳接口

#### 文件

* `include/saauso-isolate.h`

* `src/api/api-isolate.cc`

#### 改动

* 删除 `Isolate::ValueRegistrySizeForTesting() const;` 声明。

* 删除 `size_t Isolate::ValueRegistrySizeForTesting() const { return 0; }` 实现。

#### 原因

* 该接口没有真实语义，继续保留会误导测试与后续维护者。

### 2. 重写 HandleScope 相关测试为 public EscapableHandleScope 行为测试

#### 文件

* `test/embedder/embedder-script-unittest.cc`

#### 改动

* 用一个新的 public API 行为测试替换 `EmbedderPhase2Test.HandleScope_Prevents_Memory_Leak`。

* 新测试聚焦：

  * 在 helper/内层 scope 中创建临时值；

  * 通过 `EscapableHandleScope::Escape()` 仅逃逸一个 `Local<String>`；

  * 离开内层 scope 后继续进行额外分配；

  * 最终在外层读取该返回值，断言内容正确。

#### 推荐测试形态

* 测试名可调整为体现语义，如：

  * `EmbedderPhase2Test.EscapableHandleScope_EscapeValueSurvivesOuterUse`

* 该测试只验证 public 行为，不依赖 `src/` 私有头，不读内部计数。

#### 原因

* 这比“伪计数无增长”更直接验证 public `EscapableHandleScope` 的契约是否成立。

### 3. 重写 callback 压力测试为行为正确性测试

#### 文件

* `test/embedder/embedder-interop-callback-unittest.cc`

#### 改动

* 用新的行为测试替换 `EmbedderPhase3Test.Callback_EscapeScope_NoGrowth`。

* 保留“高频 callback”压力形态，但不再做 registry size 断言。

* 新测试聚焦：

  * 脚本循环多次调用宿主 callback；

  * 执行结束后检查 `TryCatch` 未捕获异常；

  * 验证 callback 造成的宿主状态或 context 中的结果值正确；

  * 验证压力结束后仍可继续调用 context `Set/Get` 或后续脚本执行，确认无状态污染。

#### 推荐测试形态

* 测试名可调整为：

  * `EmbedderPhase3Test.Callback_Stress_RemainsCorrectAfterRepeatedCalls`

* 可直接复用已有 callback fixture / helper，例如现有 `HostSetStatus`。

#### 原因

* 高频 callback 的核心验收应是“行为仍正确、状态未污染、异常未泄漏”，而不是错误的计数断言。

### 4. 清理仓库内调用点并保持测试命名清晰

#### 文件

* `test/embedder/embedder-script-unittest.cc`

* `test/embedder/embedder-interop-callback-unittest.cc`

#### 改动

* 删除所有 `ValueRegistrySizeForTesting()` 调用。

* 统一测试命名，让名称和真实验收目标一致，不再使用 `Leak` / `NoGrowth` 这类需要内部计数支撑的表述。

#### 原因

* 测试名与测试内容需要一致，否则仍会误导维护者。

## Verification

### 必做验证

1. 重新生成 debug 构建文件。
2. 编译 `embedder_ut`。
3. 运行 `embedder_ut`，确认全部通过。
4. 检索仓库，确认不再存在 `ValueRegistrySizeForTesting` 的声明、实现和调用。

### 建议附加验证

1. 如成本可接受，编译 `all_ut`，确认聚合入口未受影响。
2. 如本地环境具备 ASAN 条件，可按 `docs/Saauso-Build-and-Test-Guide.md` 的 `is_asan=true` 路径补做一次构建或说明后续建议执行。

## Implementation Notes

* 实施时先删除接口，再立即同步改测试，避免中间状态下 `embedder_ut` 编译失败。

* 重写测试时优先复用现有 `Context`、`TryCatch`、`HostSetStatus` 等辅助代码，不额外扩张测试基建。

* 保持改造范围最小：本次不新增 public API，不引入新的 test-only internal bridge。

