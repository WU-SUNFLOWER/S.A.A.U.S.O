# S.A.A.U.S.O Embedder API (MVP) 可落地实施方案（修订版）

## 1. 背景、目标与范围

S.A.A.U.S.O 的核心定位之一是作为轻量级脚本引擎供游戏引擎、办公软件等宿主程序嵌入使用，并支持嵌入方按需裁剪与扩展。当前 VM 内部基础能力（`Isolate`、`HandleScope`、`MaybeHandle`、`Factory`、`ExceptionState`、`Runtime_Execute*`）已具备构建 Embedder API 的前提。

本修订版目标是将原方案从“技术方向正确”提升为“可直接执行”，重点补齐：

1. 可见性边界与构建目标的硬门禁。
2. TryCatch/Context 等关键语义的明确契约。
3. 每阶段可验证、可回归、可交付的验收标准。

## 2. 当前现状与差距（作为实施基线）

### 2.1 已具备能力

1. Isolate 生命周期、线程进入与作用域机制已成熟。
2. 句柄与作用域体系完整（`Handle`/`HandleScope`/`MaybeHandle`）。
3. 编译与执行链路可复用（`Compiler`、`Runtime_Execute*`）。
4. 异常状态集中在 `ExceptionState`，具备清理与传播基础。

### 2.2 当前缺口

1. 对外 API 仍极薄，`include/saauso.h` 仅初始化相关接口。
2. 尚无 V8-like 的 `Local<T>`、`Script`、`TryCatch`、`Context` 对外桥接层。
3. 公开/内部头文件边界未硬化，存在外部误用 `src/**` 的风险。
4. 缺少 `test/embedder` 与 `examples/embedder` 的独立验证闭环。

## 3. MVP 设计原则与非目标

### 3.1 设计原则

1. **对外稳定语义优先**：先稳定 API 行为，再扩展类型覆盖面。
2. **内部零重构优先桥接**：MVP 只做边界桥接，不进行解释器核心重写。
3. **失败显式化**：所有可失败路径必须通过 `Maybe*` 或 `TryCatch` 可观察。
4. **可裁剪构建**：Embedder 能按构建选项裁剪能力且不破坏核心接口。

### 3.2 MVP 非目标

1. 不承诺首版二进制 ABI 长期稳定。
2. 不在 MVP 阶段实现全量 Python 类型映射。
3. 不在 MVP 阶段引入复杂多上下文并发调度模型。

## 4. 对外 API 形态（MVP）

通过 `include/saauso.h` + `include/saauso-embedder.h` 暴露最小可用接口。`saauso.h` 继续承载全局初始化；新增 `saauso-embedder.h` 承载嵌入 API，避免单头膨胀。

### 4.1 基础设施

1. `saauso::Isolate`

   * `static Isolate* New(const IsolateCreateParams&)`

   * `void Dispose()`
2. `saauso::HandleScope`

   * 栈上 RAII，对应内部 `HandleScopeImplementer`。
3. `saauso::Local<T>`

   * 仅作轻量句柄包装，禁止暴露底层 `Tagged<T>`。
4. `saauso::Context`（MVP 建议纳入）

   * `static Local<Context> New(Isolate*)`

   * 提供 `Enter()`/`Exit()` 或 `ContextScope`。

### 4.2 Value 层次（MVP 最小集合）

1. `Value`：类型判断（`IsString/IsInteger/IsDict/...`）。
2. 基础类型：`String`、`Integer`、`Boolean`。
3. 最小容器：`Object`、`Dict`（`List/Tuple` 放入 Phase 3 扩展）。
4. `Function`：`Call(context, receiver, argc, argv)`。

### 4.3 脚本执行

1. `Script::Compile(isolate, source, origin?) -> MaybeLocal<Script>`
2. `Script::Run(context) -> MaybeLocal<Value>`

说明：MVP 阶段统一返回 `MaybeLocal<T>`，避免“返回空值 + 异常状态”二义性。

### 4.4 异常处理契约（必须冻结）

`TryCatch` 语义定义如下：

1. 生效范围：构造后到析构前，当前线程、当前 isolate。
2. 捕获行为：当 API 调用失败且 isolate 有 pending exception 时，标记已捕获。
3. 清理行为：`TryCatch::Reset()` 清理捕获；`ExceptionState::Clear()` 由 API 内部在“已消费异常”时调用，不允许静默吞错。
4. 重抛行为：提供 `ReThrow()`，把已捕获异常重新设为 pending。
5. 一致性规则：`MaybeLocal` 失败时，若存在 `TryCatch`，必须可见 `HasCaught()==true`；否则异常保持 pending 交给上层处理。

## 5. 目录与构建改造（先决条件）

### 5.1 目录规划

1. `include/`

   * `saauso.h`

   * `saauso-embedder.h`
2. `src/api/`

   * `api.cc`（桥接实现）

   * `api-inl.h`（仅内部使用）
3. `test/embedder/`

   * `embedder-isolate-unittest.cc`

   * `embedder-script-unittest.cc`

   * `embedder-interop-unittest.cc`
4. `examples/embedder/`

   * `hello_world.cc`

   * `host_callback.cc`

### 5.2 构建规则（硬门禁）

1. 新增 `embedder_api` 目标（链接 `saauso_core`）。
2. 为外部示例目标移除对 `src/**` 的 include 权限，只保留 `include/`。
3. 新增“头文件边界检查”目标：若 embedder 示例直接包含 `src/**`，构建失败。
4. `vm` 保持独立可执行，避免 CLI 依赖反向污染 API 层。

## 6. 分阶段实施 Roadmap（含 Gate）

### Phase 0：基线校验与门禁建立

**目标**：在不改核心解释器前提下，完成可落地的工程先决条件。

**任务**：

1. 固化“公开头不可包含 `src/**`”规则。
2. 调整 `BUILD.gn`，引入 `embedder_api` 与 `test/embedder` 目标骨架。
3. 形成 TryCatch 契约文档（本文件第 4.4 为唯一基线）。

**Gate 0（必须通过）**：

1. `embedder` 示例只包含 `include/**` 也能通过编译。
2. 人为在示例中 `#include "src/..."` 时构建应失败。

### Phase 1：基础设施与最小生命周期

**目标**：跑通最小“创建-进入-释放”流程。

**任务**：

1. 实现 `Isolate::New/Dispose` 对外桥接。
2. 实现 `HandleScope` 与 `Local<T>` 基础封装。
3. 实现 `Context::New` 与最小 Enter/Exit 机制。

**验证**：

1. `embedder-isolate-unittest` 覆盖重复创建销毁与嵌套 scope。
2. 内存检查工具验证无明显泄漏（优先 ASan，可选 Valgrind）。

**Gate 1（必须通过）**：

1. 测试可稳定运行至少 1000 次创建销毁循环。
2. 无崩溃、无 use-after-free、无 handle 泄漏告警。

### Phase 2：脚本编译执行与异常闭环（MVP 核心）

**目标**：宿主可执行脚本并可靠接收异常。

**任务**：

1. 实现 `String/Integer/Boolean` 与 `Value` 的最小互转。
2. 实现 `Script::Compile` 与 `Script::Run`。
3. 实现 `TryCatch` 并打通 `ExceptionState`。

**验证**：

1. `hello_world.cc` 成功运行。
2. 语法错误 + 运行时错误均能被 `TryCatch` 捕获。
3. `MaybeLocal` 与 `TryCatch` 一致性规则覆盖单测。

**Gate 2（必须通过）**：

1. 正常脚本路径零异常，错误脚本路径异常可见且可清理。
2. 清理后 isolate 可继续执行后续脚本（无“脏异常态”残留）。

### Phase 3：数据交互与 C++ 回调

**目标**：完成 C++ ↔ Python 双向通信最小闭环。

**任务**：

1. 增加 `Object/Dict/List/Tuple/Function` 的 MVP 级接口。
2. 提供 `FunctionCallbackInfo` 与 native callback 注册路径。
3. 统一 fallible API 使用规范（涉及 `__hash__/__eq__` 的字典路径必须走 `Maybe`）。

**验证**：

1. Host Mock Test：`Host_GetConfig` 与 `Host_SetStatus` 双向校验通过。
2. callback 抛错时可由 `TryCatch` 在宿主侧接收并恢复。

**Gate 3（必须通过）**：

1. 至少 1 组双向调用回归用例稳定通过。
2. callback 异常、参数类型错误均有稳定错误语义。

### Phase 4：发布就绪（健壮性与文档）

**目标**：达到“第三方可接入”的发布标准。

**任务**：

1. 补全 Doxygen 风格 API 注释与嵌入教程。
2. 增加参数校验与线程访问保护（多线程 isolate 误用检测）。
3. 提供 `examples/embedder` 与最小 CMake 消费示例。
4. 增加 GC 压力与长稳测试。

**Gate 4（必须通过）**：

1. 外部最小 CMake 工程能独立编译并运行示例。
2. 压力测试下无崩溃、无明显资源增长异常。

## 7. 测试矩阵与验收标准

### 7.1 测试分层

1. **API 单测**：`test/embedder/*`，验证契约与边界。
2. **示例回归**：`examples/embedder/*`，验证真实接入路径。
3. **稳定性测试**：循环创建销毁、异常风暴、GC 压力。

### 7.2 最终验收清单（全部必须满足）

1. 对外头文件不暴露内部对象布局与内部头依赖。
2. `Isolate + HandleScope + Context + Script + TryCatch` 全链路可用。
3. 错误语义一致：`MaybeLocal` 失败与 `TryCatch` 可观测状态一致。
4. 具备最小 callback 双向通信能力并有自动化回归。
5. 外部工程可复制接入（含示例与构建说明）。

## 8. 风险清单与缓解策略

1. **风险：边界失真（embedder 误依赖内部头）**

   * 缓解：构建门禁 + 示例目标 include 白名单。
2. **风险：异常状态清理不一致**

   * 缓解：统一 `TryCatch` 契约，单测覆盖“捕获/重抛/清理”。
3. **风险：Context 生命周期混乱**

   * 缓解：限制 MVP 为单线程显式 Enter/Exit，后续再扩展并发模型。
4. **风险：接口增长过快导致维护负担**

   * 缓解：按 Gate 推进，未达 Gate 不进入下一阶段。

## 9. 里程碑交付物

### M0（Phase 0 完成）

1. 构建门禁生效。
2. 契约文档冻结。

### M1（Phase 1 完成）

1. 基础 API 头文件与桥接实现。
2. 生命周期单测通过。

### M2（Phase 2 完成）

1. 脚本编译执行 API。
2. TryCatch 异常闭环。
3. hello\_world 示例。

### M3（Phase 3 完成）

1. callback 互调能力。
2. Host Mock Test 全通过。

### M4（Phase 4 完成）

1. 发布文档与示例齐全。
2. 外部工程接入验证通过。

## 10. 执行准则（强制）

1. 任一 Phase 未通过 Gate，不得进入下一 Phase。
2. 所有新增 API 先补测试再扩功能。
3. API 层禁止直接泄漏 `Tagged<T>`、`Klass`、`Factory` 到公开头。
4. 所有可失败路径必须有明确错误传播语义，禁止 silent failure。

