# embedder-interop-unittest 拆分执行方案

## 1) Summary

目标：将 `test/embedder/embedder-interop-unittest.cc` 按“测试职责”进行拆分，降低单文件耦合和后续维护成本，同时保持测试行为、测试名、编译目标与回归结果不变。

成功标准：

* `embedder_ut` 目标可正常编译链接。

* 原有 18 个测试全部保留且断言逻辑不变。

* 拆分后每个文件聚焦单一互操作主题，便于后续定位与扩展。

## 2) Current State Analysis

已确认的仓库事实：

* 当前文件 `test/embedder/embedder-interop-unittest.cc` 同时承载多类职责：回调互操作、Context 隔离、对象容器与基础值 API、异常捕获、并行回归、`SAAUSO_ENABLE_CPYTHON_COMPILER` 条件分支测试。

* 文件内共有 18 个 `TEST(...)`（`EmbedderPhase3Test` + `EmbedderPhase4Test` 混排）。

* `test/embedder/BUILD.gn` 当前将该文件作为 `embedder_ut` 的单一 source 之一；同目录已存在按主题拆分的测试文件（`embedder-script-unittest.cc`、`embedder-isolate-unittest.cc`、`embedder-handle-thread-unittest.cc`）。

* 结论：该文件职责范围偏宽，具备高价值且低风险的“纯搬迁式拆分”条件。

## 3) Proposed Changes

### 3.1 文件拆分策略（新增 3 个测试文件）

1. `test/embedder/embedder-interop-callback-unittest.cc`

* 放置内容：

  * Host 回调 helper：`HostSetStatus`、`HostGetStatusWithValidation`、`HostAccumulateParallel`、`HostInnerPlusOne`、`HostOuterReentrant`

  * `#if SAAUSO_ENABLE_CPYTHON_COMPILER` 下的 `HostGetConfig`

  * 以“回调互操作/回调异常/并行回调/重入调用”为核心的测试用例（原 Phase3 的 callback 相关 + Phase4 的 reentrant）

* 原因：将 Host callback 与互调语义聚合，减少与对象/Context API 主题交叉。

* 实施方式：从原文件逐段搬迁，保持函数签名、测试名、断言文本不变。

1. `test/embedder/embedder-interop-context-unittest.cc`

* 放置内容：

  * Context 与 Global 桥接、多 Context 变量隔离、callback 绑定隔离、`#if` 下异常隔离、Context miss 场景

* 原因：将“作用域/上下文隔离语义”独立，形成可单独演进的测试模块。

* 实施方式：保留原有 `TryCatch` 与 `Script::Compile/Run` 调用顺序，避免行为漂移。

1. `test/embedder/embedder-interop-value-unittest.cc`

* 放置内容：

  * `Object/List/Tuple/Float/Boolean` round-trip

  * 越界访问安全返回

  * `Object::CallMethod` 命中/未命中

  * `Isolate::ThrowException` 被 `TryCatch` 捕获

* 原因：以“值与对象 API 合同”聚合，降低与 callback/context 主题耦合。

* 实施方式：保持测试主体代码与断言原样，仅移动文件位置与必要的局部 helper（若有）。

### 3.2 原文件处理

`test/embedder/embedder-interop-unittest.cc`

* 处理方式：删除已搬迁测试内容，最终删除该文件（或在过渡提交中临时留空后再删）。

* 原因：避免重复编译同名测试导致冲突，确保职责边界清晰。

### 3.3 构建文件更新

`test/embedder/BUILD.gn`

* 更新 `sources`：

  * 移除 `./embedder-interop-unittest.cc`

  * 新增：

    * `./embedder-interop-callback-unittest.cc`

    * `./embedder-interop-context-unittest.cc`

    * `./embedder-interop-value-unittest.cc`

* 保持 `deps/configs` 不变。

### 3.4 代码与命名约束

* 保持现有测试套件名（`EmbedderPhase3Test`/`EmbedderPhase4Test`）不改，避免影响筛选与历史对比。

* 不引入新的公用头/工具文件，先采用“最小变更”策略；若后续出现明显重复再独立提案做 helper 收敛。

* 不改任何断言语义，不改 `#if SAAUSO_ENABLE_CPYTHON_COMPILER` 宏边界。

## 4) Assumptions & Decisions

已锁定决策：

* 这是一次“结构重组”而非“行为改造”，目标是 SRP 收敛与可维护性提升。

* 采用“三文件拆分”而非更多碎片文件，平衡 SRP 与可读性。

* 拆分边界优先按“测试主题”而非“Phase 编号”划分，因为 Phase3/Phase4 在语义上有交叉但在职责上可归并。

* 若出现局部 helper 在多个新文件重复，当前接受少量重复，优先保证迁移稳定。

## 5) Verification Steps

1. 构建验证

* 重新生成并构建 `embedder_ut`（按仓库现有 GN/Ninja 流程）。

* 期望：编译与链接通过，无重复符号、无缺失 source。

1. 测试验证

* 运行 `embedder_ut` 全量。

* 期望：与拆分前等价通过（至少原 18 个用例全部存在并通过）。

1. 冒烟检查

* 按测试过滤器分别运行新拆分文件中的代表性用例：

  * callback 互调与重入

  * context 隔离

  * value/object API 与异常捕获

* 期望：分组运行结果与全量一致。

1. 变更审计

* 检查新增/删除文件列表与 `BUILD.gn` 一致。

* 抽样比对迁移前后测试体，确认仅结构搬迁、无语义改动。

