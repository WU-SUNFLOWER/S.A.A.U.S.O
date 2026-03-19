# S.A.A.U.S.O Embedder API（Phase 3）开发进度与架构设计说明

## 1. 文档目标

本文档用于 Phase 3 交付后的架构审查与 CR 对齐，聚焦：

1. Phase 3 目标达成情况与边界。
2. 回调互调链路（C++ ↔ Python）的设计与异常契约。
3. 生命周期管理与测试覆盖现状。

## 2. Phase 3 结论

当前已完成 Phase 3 第一批最小闭环能力：

1. `Function` 对外创建与调用能力。
2. `FunctionCallbackInfo` 参数读取与返回值设置能力。
3. `Context::Set/Get` 数据注入与读取能力。
4. `callback 抛错 -> Python -> 外层 TryCatch` 异常穿透链路。

对应验证已在前后端配置下通过。

## 3. API 增量（对外）

本阶段主要扩展 `include/saauso-embedder.h`：

1. `using FunctionCallback = void (*)(FunctionCallbackInfo& info)`
2. `class Function`
   - `static Local<Function> New(Isolate*, FunctionCallback, std::string_view name)`
   - `MaybeLocal<Value> Call(Local<Context>, Local<Value> receiver, int argc, Local<Value> argv[])`
3. `class FunctionCallbackInfo`
   - `int Length() const`
   - `Local<Value> operator[](int index) const`
   - `bool GetIntegerArg(int index, int64_t* out) const`
   - `bool GetStringArg(int index, std::string* out) const`
   - `Local<Value> Receiver() const`
   - `Isolate* GetIsolate() const`
   - `void ThrowRuntimeError(std::string_view message) const`
   - `void SetReturnValue(Local<Value> value)`
4. `class Context`
   - `bool Set(Local<String> key, Local<Value> value)`
   - `MaybeLocal<Value> Get(Local<String> key)`

## 4. 核心架构设计

### 4.1 回调注册与分发（最小可落地）

为在不重构解释器核心的前提下落地 callback，本阶段采用“固定槽位 + trampoline”策略：

1. 维护固定大小回调槽位数组（16 个）。
2. `Function::New` 时分配槽位，将 `FunctionCallback` 与 `Isolate` 绑定。
3. 使用 `NativeFuncPointer` trampoline 把 VM 调用路由到指定槽位。
4. 在 `Isolate::Dispose` 中回收该 Isolate 对应槽位，避免悬挂引用。

该方案优点是改动范围小、风险可控，缺点是槽位容量有限（后续可升级为动态注册表）。

### 4.2 FunctionCallbackInfo 桥接

`FunctionCallbackInfoImpl` 内部持有：

1. `Isolate*`
2. `receiver`
3. `args`（`PyTuple`）
4. `return_value`（`Local<Value>`）

回调返回后：

1. 若 callback 设置了 pending exception，则立即返回失败（`kNullMaybeHandle`），由上层执行链路传播。
2. 否则将 `return_value` 转换为内部对象返回给 Python 侧。

### 4.3 Context 数据交互

`Context::Set/Get` 当前直接作用于 `globals` 字典：

1. `Set`：key/value 转换后写入 `PyDict::Put`
2. `Get`：从 globals 读取，成功后包装为 `Value`
3. 失败路径统一走 `CapturePendingException`（与 TryCatch 契约对齐）

### 4.4 异常穿透契约

新增关键保证：

1. callback 调用 `FunctionCallbackInfo::ThrowRuntimeError(...)` 后，异常进入内部 `ExceptionState`。
2. trampoline 发现 pending exception 后返回失败，不再返回普通值。
3. Python 执行链路失败后，外层 `Script::Run` 返回空 `MaybeLocal`，并由外层 `TryCatch` 捕获。

这保证了“宿主回调抛错”可跨层稳定传播。

## 5. 生命周期策略（Phase 3）

在 Phase 2 已完成的 Value 生命周期治理基础上，Phase 3 保持以下原则：

1. API 创建的包装对象统一进入 Isolate 注册表。
2. `HandleScope` 负责回收本作用域新增 Value 包装对象。
3. `Isolate::Dispose` 负责最终兜底回收。
4. callback 槽位在 `Isolate::Dispose` 时解绑，避免跨 Isolate 脏引用。

## 6. 测试与验证状态

### 6.1 新增 Phase 3 用例

位于 `test/embedder/embedder-interop-unittest.cc`：

1. `FunctionCall_DirectRoundTrip`
2. `FunctionCallbackInfo_ArgumentValidation`
3. `HostMock_PythonToCppRoundTrip`（前端启用）
4. `CallbackThrow_PythonToTryCatch`（前端启用）

### 6.2 回归结果

1. Backend（`saauso_enable_cpython_compiler=false`）：
   - `embedder_ut` 通过（含参数校验用例）
2. Frontend（启用 CPython 前端）：
   - `embedder_ut` 全量通过（含 Host Mock + 异常穿透）

## 7. 风险与后续建议

### 7.1 已知限制

1. 回调槽位数量固定（16），暂不支持高并发动态扩容。
2. `Context::Set/Get` 目前只覆盖 globals 级能力，尚未提供命名空间隔离策略。
3. `FunctionCallbackInfo` 目前仅支持字符串/整数便捷读取。

### 7.2 建议下一步（Phase 3.1 / Phase 4 前置）

1. 将 callback 槽位从固定数组升级为可扩展注册表。
2. 增加 `GetBooleanArg`、`GetObjectArg` 等参数读取能力。
3. 增加 callback 嵌套抛错与重入压力测试。
4. 提供更清晰的错误码与错误类型分层（RuntimeError/TypeError）。

## 8. 审查入口索引

1. `include/saauso-embedder.h`
2. `src/api/api-inl.h`
3. `src/api/api.cc`
4. `test/embedder/BUILD.gn`
5. `test/embedder/embedder-interop-unittest.cc`
6. `test/embedder/embedder-script-unittest.cc`

