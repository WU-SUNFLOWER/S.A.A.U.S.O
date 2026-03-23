# S.A.A.U.S.O Embedder API 架构设计思路

## 1. 研究背景与设计目标

S.A.A.U.S.O Embedder API 的核心目标，是把脚本运行能力以“可嵌入组件”的形式交付给 C++ 宿主系统，使宿主能够在不暴露 VM 内部实现细节的前提下，安全、可控地完成脚本加载、运行、互调与错误处理。  
该设计面向两个主要读者群体：

1. 人类程序员：希望快速掌握 API 心智模型、生命周期边界与异常约束；
2. AI 助手：希望在统一、可推理的接口语义下执行检索、改造、测试与文档生成。

从工程角度，架构目标可归纳为五条主线：

1. **边界稳定**：公开头文件不泄漏内部对象布局与 VM 私有类型；
2. **生命周期可控**：通过 `Isolate::Scope + HandleScope` 双层作用域控制调用窗口与句柄回收，避免宿主侧泄漏；
3. **互调链路闭环**：C++ ↔ Python 调用路径可观测、可回退、可验证；
4. **异常语义统一**：失败以 `MaybeLocal` 与 `TryCatch` 体现，不依赖隐式崩溃；
5. **演进友好**：在 MVP 阶段允许过渡实现，但接口形态应为后续扩展预留稳定入口。

## 2. 总体架构分层

Embedder API 采用“公共 ABI 层 + 桥接层 + VM 运行层”的三层结构：

1. **公共 ABI 层（include/）**
   - 对外提供 `Isolate`、`Context`、`Script`、`Value` 及其派生类型；
   - 提供 `Function`、`FunctionCallbackInfo`、`TryCatch`、`Exception` 等互调与错误接口；
   - 约束：不暴露 `src/` 私有头与内部 `Tagged<T>` 表达。

2. **桥接层（src/api）**
   - 完成 `Local/MaybeLocal` 与 VM 对象句柄之间的双向映射；
   - 实现 `CapturePendingException`、`ToInternalObject`、`WrapRuntimeResult` 等关键转换；
   - 负责 HandleScope 生命周期管理与 API 级防御性语义。

3. **VM 运行层（src/objects + src/runtime）**
   - 提供对象系统、调用协议、异常状态与执行机制；
   - 承担 Native Function 调度、属性访问、函数调用等基础能力；
   - 对 Embedder 层透明，保持“可替换实现、稳定接口”策略。

该分层的直接收益是：接口可讲解、实现可演进、问题可定位。

## 3. 核心对象模型与生命周期治理

### 3.1 Isolate：隔离执行单元

`Isolate` 是脚本运行的最外层沙箱，内部持有独立异常状态、上下文与对象空间。  
Embedder 通过 `Isolate::New/Dispose` 控制生命周期，确保资源创建与回收呈现明确边界。

在当前设计中，`Isolate` 遵循单线程访问模型：同一实例不承诺并发调用安全。并行场景建议使用多实例隔离。

此外，Embedder 侧应优先使用 `Isolate::Scope` 作为 RAII 门面来管理 `Enter/Exit`，并将所有带 `Isolate*` 的 API 调用放在该作用域内。

### 3.2 Context：全局环境容器

`Context` 可理解为脚本执行时的全局命名空间。  
API 提供两种访问风格：

1. 快捷风格：`Context::Set/Get`
2. 对象风格：`Context::Global()` 返回 `Object`，再使用 `Set/Get/CallMethod`

当前实现中，`Context::New` 每次都会分配独立 `globals`，因此同一 `Isolate` 内的多个 Context 具备天然隔离。  
这使“从简单脚本注入”到“复杂对象互调”拥有一致升级路径，并可支撑多租户/多会话并存。

### 3.3 Context 栈语义（Enter/Exit）

Context 采用每 Isolate 一条 LIFO 栈语义：

1. `Context::Enter`：将当前 Context 压入该 Isolate 的 entered-context 栈；
2. `Context::Exit`：仅允许退出栈顶 Context，保证 Enter/Exit 配对；
3. 当前实现中，首次入栈会尝试进入内部 Isolate，栈清空时退出内部 Isolate；
4. `ContextScope` 为 `Enter/Exit` 的 RAII 门面，推荐优先使用。

该设计把“上下文切换顺序正确性”收敛为统一约束，降低了跨层调用时的状态错乱风险。
但它不替代 Embedder 层显式 `Isolate::Scope` 契约。

### 3.4 Local / MaybeLocal：显式成功-失败协议

`Local<T>` 表示已成功获取的句柄；`MaybeLocal<T>` 表示“可能失败”的操作结果。  
该机制强制调用者面对失败分支，配合 `TryCatch` 形成显式错误处理闭环，避免隐性空指针行为。

### 3.5 HandleScope / EscapableHandleScope：内存隔离策略

Embedder 层包装对象由作用域管理：

1. `HandleScope`：作用域内创建、作用域末回收；
2. `EscapableHandleScope`：允许一个结果跨作用域逃逸，其余临时对象回收。

`HandleScope` 只管理句柄生命周期，不负责 Isolate 绑定。  
这一策略与 `Isolate::Scope` 组合构成 V8-like 生命周期治理，解决“高频互调导致包装对象堆积”的工程问题。

### 3.6 Isolate 契约硬化

当前 API 层采用严格契约：

1. 带 `Isolate*` 入参的入口必须满足 `Current() == explicit isolate`；
2. 依赖隐式 Current 的入口必须满足 `Current() != nullptr`；
3. 违约场景统一按硬失败处理，避免 silent wrong-isolate 漂移；
4. 契约负向用例（无 Current、isolate 不一致、跨线程误用）纳入 CI。

## 4. 调用体系设计：C++ ↔ Python 互调链路

### 4.1 宿主函数注入

宿主通过 `Function::New` 将 C++ 回调注入脚本侧，回调签名统一为 `FunctionCallbackInfo`。  
回调中可完成参数读取、返回值设置与异常抛出（`ThrowRuntimeError`）。

### 4.2 闭包路由分发机制

为避免早期固定槽位的容量与并发限制，当前 Native Function 采用 closure_data 路由方案：

1. 函数模板携带 closure 绑定信息；
2. 运行时调用时将 closure_data 透传到 Native 回调；
3. 使用可扩展绑定表完成稳定分发。

该机制实现了从“静态槽位 Hack”到“可扩展闭包调度”的架构升级。

### 4.3 函数调用路径

`Function::Call` 在桥接层会完成：

1. 入参封装（`Local<Value>[]` -> VM tuple）
2. 调用执行（`PyObject::Call`）
3. 结果包装（VM 对象 -> `Local<Value>`）
4. 失败处理（空 `MaybeLocal` + `CapturePendingException`）

这条路径确保任何调用失败都可由外层 `TryCatch` 观测。

## 5. 数据互操作设计：Value 家族扩展策略

API 当前覆盖 `String/Integer/Float/Boolean/Object/List/Tuple`。  
其设计重点不是“类型数量”，而是“扩展一致性”：

1. 每个值类型都需提供“创建 + 读取 + 判型”闭环；
2. VM 到 API、API 到 VM 两个方向都需覆盖转换；
3. 容器类型操作（如 `List::Get`、`Object::CallMethod`）必须遵守 `MaybeLocal` 失败约定。

通过该策略，后续扩展 `Dict/Set/Bytes` 等类型时可复用同一架构模板。

## 6. 异常模型：可穿透、可捕获、可恢复

### 6.1 基础语义

Embedder API 不采用“失败即崩溃”的隐式模型，而采用：

1. 失败返回空 `MaybeLocal`
2. 运行时异常写入 pending exception
3. `CapturePendingException` 将异常转交最近 `TryCatch`

### 6.2 宿主抛错入口

目前支持两类宿主抛错：

1. 回调内：`FunctionCallbackInfo::ThrowRuntimeError`
2. 任意时刻：`Isolate::ThrowException(Local<Value>)`

配合 `Exception::TypeError/RuntimeError` 工厂，可形成更规范的错误对象构造流程。

### 6.3 防御性编程约束

架构层约定：所有 Fallible 路径必须显式兜底。  
典型做法包括：

1. 调用后检查 `HasPendingException()`
2. 失败时 `CapturePendingException`
3. 返回空 `MaybeLocal` 或 `false`

此约束显著降低了“静默失败”与“状态不一致”风险。

在严格契约下，API 前置条件违约不再采用“静默返回”策略，而是直接终止进程，优先保证状态一致性与故障可诊断性。

## 7. Demo 驱动验证策略

为验证 API 是否“可用而非仅可测”，项目采用双轨验证：

1. **单元测试轨**：覆盖参数校验、越界、重入、异常穿透、并发回归、多 Context 隔离；
2. **实战示例轨**：`game-engine-demo` 模拟主循环与 AI 调用，展示主路径失败后的 fallback 降级。

这使架构质量不只停留在函数正确性，而延伸到系统可演示性与故障可恢复性。

## 8. 已知限制与工程边界

当前 MVP 的关键边界如下：

1. OOM 不承诺可恢复，极端情况下可能直接终止进程；
2. `Isolate` 按单线程模型使用，不保证同实例并发安全；
3. 前端编译能力依赖开关与构建配置；
4. 二进制 ABI 仍在演进期，不承诺跨版本稳定。

这些限制并非缺陷回避，而是“可控承诺”策略，便于在教学与工程场景中做真实风险评估。

## 9. 面向论文与长期演进的价值

从毕业设计视角，该架构的研究价值体现在三点：

1. **方法论价值**：将 VM 嵌入问题拆解为“生命周期、互调、异常、边界”四大可验证模块；
2. **工程价值**：形成可落地的 C++ 脚本化接口，支持实战 demo 与可复制接入流程；
3. **演进价值**：以稳定接口形态承载内部实现迭代，为后续类型扩展与调用优化留出空间。

从 AI 协作视角，该架构具备高可读、高可检索与高可测试特征，适合持续自动化演进。

## 10. 结论

S.A.A.U.S.O Embedder API 的设计核心，不是追求“最短代码路径”，而是建立“可解释、可验证、可扩展”的嵌入式脚本架构。  
通过 V8-like 生命周期管理、闭包路由分发、统一异常捕获与防御性兜底机制，系统已形成从最小示例到实战 demo 的完整能力闭环，可作为教学、原型验证与工程化迭代的稳定基线。

## 11. 源码文件路径索引（CR 与自动化检索入口）

本节用于帮助人类程序员与 AI 助手在 Code Review、问题定位与功能扩展时快速跳转到关键实现。

### 11.1 对外 API 与桥接实现

1. 对外聚合头：`include/saauso.h`
2. 对外拆分头：
   - `include/saauso-isolate.h`
   - `include/saauso-local-handle.h`
   - `include/saauso-context.h`
   - `include/saauso-script.h`
   - `include/saauso-function.h`
   - `include/saauso-function-callback.h`
   - `include/saauso-exception.h`
   - `include/saauso-value.h`
   - `include/saauso-primitive.h`
   - `include/saauso-object.h`
   - `include/saauso-container.h`
3. 桥接内联工具：`src/api/api-inl.h`
4. 桥接主实现：
   - `src/api/api-isolate.cc`
   - `src/api/api-local-handle.cc`
   - `src/api/api-context.cc`
   - `src/api/api-script.cc`
   - `src/api/api-function.cc`
   - `src/api/api-function-callback.cc`
   - `src/api/api-exception.cc`
   - `src/api/api-value.cc`
   - `src/api/api-primitive.cc`
   - `src/api/api-container.cc`
   - `src/api/api-objects.cc`
   - `src/api/api-support.cc`

### 11.2 调用链与运行时核心

1. Native/Python 函数调用运行时：`src/runtime/runtime-py-function.cc`
2. 函数对象与闭包数据字段：`src/objects/py-function.h`
3. 函数类型行为与调用虚表：`src/objects/py-function-klass.cc`
4. 通用对象调用入口：`src/objects/py-object.cc`
5. 默认可调用语义（含 not callable 报错）：`src/objects/py-object-klass.cc`
6. Native function 指针签名定义：`src/objects/native-function-helpers.h`
7. 模板到函数实例的承载结构：`src/objects/templates.h`

### 11.3 异常与生命周期相关入口

1. Isolate 异常状态：`src/execution/exception-state.h`
2. 异常辅助工具：`src/execution/exception-utils.h`
3. 运行时抛错门面：`src/runtime/runtime-exceptions.h`
4. 句柄作用域实现：`src/handles/handle-scopes.h`

### 11.4 构建与示例入口

1. 根构建配置：`BUILD.gn`
2. Embedder 示例：
   - `samples/hello-world.cc`
   - `samples/game-engine-demo.cc`
   - `samples/README.md`

### 11.5 测试入口（建议先读）

1. Embedder 互操作测试：`test/embedder/embedder-interop-unittest.cc`
2. Embedder 脚本测试：`test/embedder/embedder-script-unittest.cc`
3. Embedder Isolate 测试：`test/embedder/embedder-isolate-unittest.cc`
4. 测试构建文件：`test/embedder/BUILD.gn`

### 11.6 文档入口（建议阅读顺序）

1. 接入层文档（先读）：`docs/Embedder-API-接入指南.md`
2. 架构层文档（当前文档）：`docs/Embedder-API-架构设计思路.md`
