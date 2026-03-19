# S.A.A.U.S.O Embedder API（Phase 0/1/2）开发进度与架构设计说明

## 1. 文档目的与适用范围

本文档用于支持后续 Gemini 对当前 Embedder API 产出的审查（CR），聚焦以下内容：

1. Phase 0、Phase 1、Phase 2 的实际交付范围与状态。
2. 当前架构设计、关键接口、异常契约与生命周期策略。
3. 已完成的验证与尚存风险，便于审查方快速定位重点。

本文档仅覆盖当前仓库中已落地代码，不包含未来 Phase 3/4 的实现细节。

## 2. 当前总体状态（结论先行）

截至当前提交窗口：

1. **Phase 0：已完成**（构建门禁、目标分层、embedder 测试接线完成）。
2. **Phase 1：已完成**（Isolate/HandleScope/Context 最小生命周期闭环完成）。
3. **Phase 2：核心闭环完成**（Script::Compile/Run、TryCatch、Value 类型化读取落地并可回归）。
4. **前端启用场景已打通**（`python312.lib` 已可被链接，前端用例可跑通）。

## 3. 设计目标与约束

### 3.1 目标

1. 对外提供 V8-like 的最小 Embedder API，隐藏 VM 内部对象布局细节。
2. 在尽量不重构核心解释器的前提下，通过 API 桥接快速交付可用能力。
3. 建立“可执行 + 可回归 + 可审查”的阶段化交付基线。

### 3.2 约束

1. MVP 阶段以单 Isolate、单上下文语义为主，不引入复杂并发上下文调度。
2. 对外 API 不暴露 `Tagged<T>`、`Klass`、`Factory` 等内部实现细节。
3. 所有失败路径必须可观测（`MaybeLocal` 为空 + `TryCatch` 可消费异常）。

## 4. 目录与构建层设计（Phase 0）

### 4.1 关键目标

1. 形成核心 VM、可选 CPython 前端、Embedder API 的清晰分层。
2. 增加 Embedder 边界门禁，避免示例/外部目标误引 `src/**`。
3. 把 embedder 单测并入统一回归入口，避免后续演进脱测。

### 4.2 已落地目标

1. 根构建文件完成结构化注释与分段组织（核心、前端、embedder、测试）。
2. 新增 `embedder_api` 目标与 `embedder_hello_world` 示例目标。
3. 新增 `embedder_header_boundary_check` 检查目标，约束示例 include 边界。
4. `ut` 组已纳入 embedder 检查与单测目标。

### 4.3 CPython 链接配置修复（关键）

为解决 Windows 前端模式下 `python312.lib` 链接失败，当前采用以下配置：

1. `saauso_cpython312_compiler` 依赖 `//third_party/cpython312`。
2. 通过 `public_configs + all_dependent_configs` 传播 `python312_config`，确保 `/LIBPATH` 与 `python312.lib` 进入最终可执行链接阶段。

该修复已在前端启用构建中验证通过。

## 5. API 骨架与生命周期（Phase 1）

### 5.1 对外 API 基础形态

当前公开头主要位于：

1. `include/saauso.h`：全局初始化入口。
2. `include/saauso-embedder.h`：Embedder API（Isolate/HandleScope/Local/Context 等）。

### 5.2 生命周期模型

#### 5.2.1 Isolate

1. `Isolate::New()` 创建内部 `internal::Isolate`。
2. `Isolate::Dispose()` 负责清理 Context、Value 注册对象、内部 Isolate。
3. 目前单测已恢复真实销毁断言，避免用规避方式跳过释放路径。

#### 5.2.2 HandleScope

1. 对外 `HandleScope` 使用 RAII。
2. 内部通过 `internal::Isolate::Scope + internal::HandleScope` 绑定当前线程作用域。

#### 5.2.3 Context

1. 默认 Context 在 `Isolate::New()` 阶段创建并持有 globals。
2. `ContextScope` 提供 Enter/Exit 的 RAII 包装。

## 6. Script/Value/TryCatch 设计与实现（Phase 2）

### 6.1 Script 能力

1. `Script::Compile(isolate, source)`：
   - 当前语义为将脚本源码封装为 Script 值对象（MVP 语义）。
2. `Script::Run(context)`：
   - 桥接 `Runtime_ExecutePythonSourceCode` 执行脚本。
   - 成功返回 `MaybeLocal<Value>`；失败返回空并通过 TryCatch 捕获。

### 6.2 Value 表示策略（当前版本）

当前 Value 内部使用私有实现体 + kind 分类：

1. `kHostString`
2. `kHostInteger`
3. `kScriptSource`
4. `kVmObject`

对应能力：

1. 类型判断：`IsString()` / `IsInteger()`
2. 类型化读取：`ToString(std::string*)` / `ToInteger(int64_t*)`
3. 返回值细化：`Script::Run` 对 `PyString`/`PySmi` 返回值做类型化包装，便于 embedder 直接读取。

### 6.3 TryCatch 契约（当前实现）

1. `TryCatch` 构造时入栈、析构时出栈，支持嵌套链。
2. 当 API 调用失败且存在 pending exception 时：
   - 若存在当前 `TryCatch`，在 API 边界消费异常并写入 `TryCatch::exception_`。
   - 若不存在 `TryCatch`，异常维持 pending，由上层路径处理。
3. `Reset()` 清空当前捕获状态，不影响后续捕获能力。

## 7. 生命周期治理与稳定性修复（Phase 2 收口重点）

本轮收口重点在于“把可跑通变成可销毁”：

1. 恢复 Value 注册（所有 Wrap* 创建的 Value 均进入 Isolate 注册表）。
2. 在 `Isolate::Dispose()` 中统一释放：
   - `ValueImpl`
   - `Value` 对象本体
3. 单测恢复真实 `Dispose()`，不再使用 `(void)isolate` 规避。

这部分是本轮 Phase 2 收口最核心的工程质量提升。

## 8. 测试与验证状态

### 8.1 Backend 模式（`saauso_enable_cpython_compiler=false`）

1. `embedder_ut` 通过（Phase1 + Phase2 用例）。
2. `embedder_hello_world` 可执行。

### 8.2 Frontend 模式（启用 CPython 前端）

1. 先前失败点：`python312.lib` 未进入最终链接参数。
2. 当前状态：链接配置修复后，`embedder_ut` 与 `embedder_hello_world` 均可编译并运行。
3. 前端正向用例 `ScriptRunSucceedsWithFrontendCompiler` 已通过。

### 8.3 关键用例覆盖

1. Isolate 创建销毁。
2. HandleScope + Context 循环创建。
3. String/Integer 类型化读取。
4. 前端启用场景 Script::Run 正向成功。
5. 前端关闭场景 TryCatch 异常捕获。

## 9. CR 关注点建议（给 Gemini）

建议优先审查以下维度：

1. **Value 表示策略**：`ValueImpl + kind` 是否满足中期可扩展性（后续 Float/Boolean/容器）。
2. **异常契约一致性**：`MaybeLocal` 空值与 TryCatch 捕获语义是否在所有路径一致。
3. **生命周期边界**：`Isolate::Dispose()` 的释放顺序是否可进一步强化（尤其异常中断场景）。
4. **Context 模型演进**：默认 Context 当前足够 MVP，但多 Context/安全边界需 Phase 3+ 规划。
5. **脚本编译语义**：`Script::Compile` 当前是源码封装语义，是否需要在下阶段切换为预编译对象语义。

## 10. 已知限制与后续建议

### 10.1 已知限制

1. `Script::Compile` 当前未做“预编译缓存对象”语义，而是保留源码并在 Run 阶段执行。
2. Value 仍是最小可用方案，后续类型扩展会提升复杂度。
3. 目前尚未进入 Phase 3 的 callback 注入与容器互调全量实现。

### 10.2 建议的下一步

1. 在 Phase 3 启动前，先冻结 Value 扩展策略（避免 API 抖动）。
2. 增加 TryCatch 嵌套/重入/无捕获器路径的专题测试。
3. 逐步引入 callback 最小闭环并完善 Host Mock Test。

## 11. 变更入口索引（便于快速审查）

1. `BUILD.gn`（根构建、前端链接配置传播、embedder 目标组织）
2. `include/saauso-embedder.h`（对外 API 定义）
3. `src/api/api-inl.h`（API 内部访问桥接）
4. `src/api/api.cc`（核心实现：生命周期、Script、Value、TryCatch）
5. `test/embedder/BUILD.gn`（embedder_ut 目标）
6. `test/embedder/embedder-isolate-unittest.cc`
7. `test/embedder/embedder-script-unittest.cc`
8. `examples/embedder/hello_world.cc`

---

若审查方需要，我可以基于本文档再补一份“按函数级别的审查清单（Checklist）”，把每个 API 的预期行为、失败语义和测试对应关系逐条列出，便于逐项 CR。
