# S.A.A.U.S.O Embedder API Phase 3 函数级 CR Checklist

## 1. 使用方式

本清单用于逐函数审查 Phase 3 新增能力。每项包含：

1. 预期行为
2. 失败语义
3. 对应测试

建议审查顺序：生命周期 -> 参数校验 -> 异常穿透 -> Host Mock 互调。

## 2. Context 互操作 API

### 2.1 `Context::Set(Local<String> key, Local<Value> value)`

1. 预期行为
   - 将 key/value 写入当前上下文 globals 字典。
2. 失败语义
   - key 为空、上下文无效、字典写入失败时返回 false。
   - 若内部有 pending exception，应由 TryCatch 感知。
3. 对应测试
   - `EmbedderPhase3Test.HostMock_PythonToCppRoundTrip`（注入 Host 回调）

### 2.2 `Context::Get(Local<String> key)`

1. 预期行为
   - 从 globals 读取 key 并包装为 `MaybeLocal<Value>` 返回。
2. 失败语义
   - key 为空或未命中返回空 `MaybeLocal`。
   - 内部读取异常时返回空，并可被 TryCatch 捕获。
3. 对应测试
   - 间接覆盖：HostMock 脚本依赖 `Set` 注入后读取函数名调用。
   - 待补：显式 `Get` 命中/未命中断言。

## 3. Function API

### 3.1 `Function::New(Isolate*, FunctionCallback, std::string_view name)`

1. 预期行为
   - 注册 callback 槽位，创建可调用函数对象。
2. 失败语义
   - isolate/callback 为空返回空 Local。
   - 槽位耗尽时抛 RuntimeError，并返回空 Local。
3. 对应测试
   - `EmbedderPhase3Test.FunctionCall_DirectRoundTrip`
   - `EmbedderPhase3Test.HostMock_PythonToCppRoundTrip`

### 3.2 `Function::Call(...)`

1. 预期行为
   - 构造参数元组并调用函数，成功返回 `MaybeLocal<Value>`。
2. 失败语义
   - 上下文无效、函数对象无效、执行失败时返回空 `MaybeLocal`。
   - callback 抛错时，错误应穿透并可由外层 TryCatch 捕获。
3. 对应测试
   - `EmbedderPhase3Test.FunctionCall_DirectRoundTrip`
   - `EmbedderPhase3Test.FunctionCallbackInfo_ArgumentValidation`
   - `EmbedderPhase3Test.CallbackThrow_PythonToTryCatch`

## 4. FunctionCallbackInfo API

### 4.1 `Length()`

1. 预期行为
   - 返回 callback 入参数量。
2. 失败语义
   - impl 无效时返回 0。
3. 对应测试
   - `FunctionCallbackInfo_ArgumentValidation` 间接覆盖。

### 4.2 `operator[](int index)`

1. 预期行为
   - 返回指定参数包装值。
2. 失败语义
   - 越界或 impl 无效时返回空 Local。
3. 对应测试
   - `FunctionCall_DirectRoundTrip`、`FunctionCallbackInfo_ArgumentValidation` 间接覆盖。

### 4.3 `GetIntegerArg(int index, int64_t* out)`

1. 预期行为
   - 参数存在且可转整数时返回 true，并写入 out。
2. 失败语义
   - out 为空、越界、类型不匹配返回 false。
3. 对应测试
   - `FunctionCall_DirectRoundTrip`（成功路径）
   - `FunctionCallbackInfo_ArgumentValidation`（失败路径）

### 4.4 `GetStringArg(int index, std::string* out)`

1. 预期行为
   - 参数存在且可转字符串时返回 true，并写入 out。
2. 失败语义
   - out 为空、越界、类型不匹配返回 false。
3. 对应测试
   - `FunctionCallbackInfo_ArgumentValidation`
   - `HostMock_PythonToCppRoundTrip`

### 4.5 `Receiver()`

1. 预期行为
   - 返回当前回调 receiver 包装值。
2. 失败语义
   - impl 无效返回空 Local。
3. 对应测试
   - 待补：显式 receiver 语义断言。

### 4.6 `GetIsolate()`

1. 预期行为
   - 返回当前 callback 对应 Isolate。
2. 失败语义
   - impl 无效返回 nullptr。
3. 对应测试
   - 间接覆盖：所有 callback 内用该接口创建返回值。

### 4.7 `ThrowRuntimeError(std::string_view message)`

1. 预期行为
   - 向内部 ExceptionState 写入 RuntimeError，触发失败传播。
2. 失败语义
   - impl/isolate 无效时 no-op。
3. 对应测试
   - `FunctionCallbackInfo_ArgumentValidation`
   - `CallbackThrow_PythonToTryCatch`

### 4.8 `SetReturnValue(Local<Value> value)`

1. 预期行为
   - 设置 callback 返回值，供 trampoline 返回给 Python 侧。
2. 失败语义
   - impl 无效时忽略。
3. 对应测试
   - `FunctionCall_DirectRoundTrip`
   - `HostMock_PythonToCppRoundTrip`

## 5. 异常穿透链路审查点

请重点确认以下链路完整性：

1. callback 调用 `ThrowRuntimeError`
2. trampoline 检测 pending exception 并返回失败句柄
3. Python 执行失败返回空 `MaybeLocal`
4. 外层 `TryCatch` 感知 `HasCaught==true`

对应测试：`EmbedderPhase3Test.CallbackThrow_PythonToTryCatch`

## 6. 生命周期审查点

1. callback 创建的返回值是否纳入 `HandleScope` 生命周期管理。
2. `Isolate::Dispose` 是否清理 callback 槽位绑定，避免悬挂 callback。
3. `HandleScope` 对 Value 注册表的回收是否与 Phase 2 契约一致。

## 7. 建议补测项

1. `Function::New` 槽位耗尽路径（16+ 回调）错误语义。
2. `Context::Get` 未命中/命中显式断言。
3. callback 嵌套 callback 的重入与异常优先级。
4. `Receiver()` 的可见性与语义稳定性。
