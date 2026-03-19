# S.A.A.U.S.O Embedder API 函数级 CR Checklist（Phase 0/1/2）

## 1. 使用说明

本清单面向审查者（Gemini/人类 Reviewer），用于逐 API 进行一致性检查。每项包含：

1. 预期行为（成功语义）。
2. 失败语义（空值、异常、状态变化）。
3. 对应测试（已覆盖 / 待补）。

审查建议顺序：先看生命周期，再看异常契约，最后看类型化读取与执行语义。

## 2. 全局初始化 API（`include/saauso.h`）

### 2.1 `Saauso::Initialize()`

1. **预期行为**
   - 初始化全局 VM 运行时，使后续 `Isolate::New()` 可用。
2. **失败语义**
   - 当前对外无返回值；异常由内部机制处理。
3. **对应测试**
   - 间接覆盖：`EmbedderPhase1Test.CreateDisposeIsolate`
   - 间接覆盖：`EmbedderPhase1Test.CreateHandleScopeAndContext`

### 2.2 `Saauso::Dispose()`

1. **预期行为**
   - 清理全局 VM 运行时资源，配对 `Initialize()`。
2. **失败语义**
   - 当前对外无返回值；调用时序错误由内部约束承担。
3. **对应测试**
   - 间接覆盖：所有 embedder 用例结尾都调用 `Saauso::Dispose()`。

### 2.3 `Saauso::IsInitialized()`

1. **预期行为**
   - 返回全局初始化状态。
2. **失败语义**
   - 无异常语义，纯状态读取。
3. **对应测试**
   - **待补**：当前 embedder 测试未直接断言该函数。

## 3. 生命周期与作用域 API（`include/saauso-embedder.h`）

### 3.1 `Isolate::New(const IsolateCreateParams&)`

1. **预期行为**
   - 创建外部 Isolate 包装对象。
   - 创建内部 `internal::Isolate`。
   - 初始化默认 `Context` 与 globals。
2. **失败语义**
   - 内部创建失败返回 `nullptr`。
3. **对应测试**
   - 已覆盖：`EmbedderPhase1Test.CreateDisposeIsolate`
   - 已覆盖：`EmbedderPhase1Test.CreateHandleScopeAndContext`

### 3.2 `Isolate::Dispose()`

1. **预期行为**
   - 释放默认 Context 与其私有实现。
   - 遍历注册表释放全部 `ValueImpl` + `Value` 对象。
   - 释放内部 `internal::Isolate` 并销毁外部对象自身。
2. **失败语义**
   - 当前无返回值；若存在未退出作用域对象，行为依赖内部断言与约束。
3. **对应测试**
   - 已覆盖：`EmbedderPhase1Test.CreateDisposeIsolate`
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`
   - 已覆盖：Phase2 前端/后端脚本测试结束后的 `Dispose()`

### 3.3 `HandleScope::HandleScope(Isolate*)` / `~HandleScope()`

1. **预期行为**
   - 构造：建立当前线程 Isolate 作用域并打开内部 HandleScope。
   - 析构：关闭对应作用域，保证 RAII 配对。
2. **失败语义**
   - 传入空 Isolate 会导致内部 unwrap 为空，行为不应被业务依赖。
3. **对应测试**
   - 已覆盖：`EmbedderPhase1Test.CreateHandleScopeAndContext`（1000 次循环）
   - 已覆盖：Phase2 全部测试均在局部块中验证作用域析构顺序。

### 3.4 `Context::New(Isolate*)`

1. **预期行为**
   - 返回 Isolate 默认 Context 的 `Local<Context>`。
2. **失败语义**
   - 传入空 Isolate 返回空 Local。
3. **对应测试**
   - 已覆盖：`EmbedderPhase1Test.CreateHandleScopeAndContext`
   - 已覆盖：Phase2 `Script::Run` 用例均通过 `Context::New(isolate)` 执行。

### 3.5 `Context::Enter()` / `Context::Exit()`

1. **预期行为**
   - 分别调用内部 Isolate Enter/Exit，控制执行上下文进入与退出。
2. **失败语义**
   - 若内部 Isolate 为空则 no-op。
3. **对应测试**
   - 间接覆盖：`ContextScope` RAII 测试路径。

### 3.6 `ContextScope::ContextScope(Local<Context>)` / `~ContextScope()`

1. **预期行为**
   - 构造时 Enter，析构时 Exit。
2. **失败语义**
   - 传入空 Context 时 no-op。
3. **对应测试**
   - 已覆盖：`EmbedderPhase1Test.CreateHandleScopeAndContext`

## 4. Value / Local / MaybeLocal API

### 4.1 `Local<T>::IsEmpty()`

1. **预期行为**
   - 判断当前 `Local` 是否为空句柄。
2. **失败语义**
   - 无异常语义。
3. **对应测试**
   - 已覆盖：各测试对 `source/text/number/context` 的 `ASSERT_FALSE(IsEmpty())`。

### 4.2 `Local<T>::Cast(Local<S>)`

1. **预期行为**
   - 执行静态转换，供 `Local<Value>::Cast(text)` 等用法。
2. **失败语义**
   - 类型不匹配由调用方承担，当前不做运行时校验。
3. **对应测试**
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`

### 4.3 `MaybeLocal<T>::IsEmpty() / ToLocal(...) / ToLocalChecked()`

1. **预期行为**
   - `IsEmpty`：判断是否为空结果。
   - `ToLocal`：安全导出 Local，失败返回 false。
   - `ToLocalChecked`：直接导出 Local（调用方保证非空）。
2. **失败语义**
   - 空结果时 `ToLocal` 返回 false。
3. **对应测试**
   - 已覆盖：`ScriptRunSucceedsWithFrontendCompiler`（`ToLocal` 成功）
   - 已覆盖：后端异常路径（`run_result.IsEmpty()`）

### 4.4 `Value::IsString()` / `Value::IsInteger()`

1. **预期行为**
   - 基于内部 kind 或 VM 对象类型判断字符串/整数语义。
2. **失败语义**
   - 非目标类型返回 false，不抛异常。
3. **对应测试**
   - 间接覆盖：`ToString/ToInteger` 的行为断言。
   - **待补**：直接断言 `IsString/IsInteger` 的显式用例。

### 4.5 `Value::ToString(std::string*)`

1. **预期行为**
   - 若值可按字符串语义读取，写入 out 并返回 true。
2. **失败语义**
   - `out == nullptr` 或类型不匹配时返回 false。
3. **对应测试**
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`
   - 已覆盖：前端脚本正向测试中对非字符串结果的 false 断言。

### 4.6 `Value::ToInteger(int64_t*)`

1. **预期行为**
   - 若值可按整数语义读取，写入 out 并返回 true。
2. **失败语义**
   - `out == nullptr` 或类型不匹配时返回 false。
3. **对应测试**
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`
   - 已覆盖：前端脚本正向测试中对非整数结果的 false 断言。

## 5. 基础类型 API

### 5.1 `String::New(Isolate*, std::string_view)`

1. **预期行为**
   - 创建字符串值包装对象并注册到 Isolate 生命周期管理。
2. **失败语义**
   - `isolate == nullptr` 时返回空 Local。
3. **对应测试**
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`
   - 已覆盖：脚本测试中的 source 构建路径。

### 5.2 `String::Value()`

1. **预期行为**
   - 返回字符串内容。
2. **失败语义**
   - 不可读取时返回空串。
3. **对应测试**
   - 间接覆盖：`ToString` 路径。
   - **待补**：直接调用 `String::Value()` 的显式断言。

### 5.3 `Integer::New(Isolate*, int64_t)`

1. **预期行为**
   - 创建整数值包装对象并注册到 Isolate 生命周期管理。
2. **失败语义**
   - `isolate == nullptr` 时返回空 Local。
3. **对应测试**
   - 已覆盖：`EmbedderPhase2Test.StringAndIntegerRoundTrip`

### 5.4 `Integer::Value()`

1. **预期行为**
   - 返回整数内容。
2. **失败语义**
   - 不可读取时返回 0。
3. **对应测试**
   - 间接覆盖：`ToInteger` 路径。
   - **待补**：直接调用 `Integer::Value()` 的显式断言。

## 6. Script API

### 6.1 `Script::Compile(Isolate*, Local<String>)`

1. **预期行为**
   - 将脚本文本封装为 Script 对象（当前 MVP 语义）。
2. **失败语义**
   - Isolate 为空、source 为空或 source 非字符串时返回空 `MaybeLocal<Script>`。
3. **对应测试**
   - 已覆盖：前端正向测试与后端异常测试中的 compile 路径。

### 6.2 `Script::Run(Local<Context>)`

1. **预期行为**
   - 在 Context globals 上执行脚本。
   - 成功返回 `MaybeLocal<Value>`。
   - 对 `PyString/PySmi` 结果做类型化包装。
2. **失败语义**
   - context/script/context_impl 非法时返回空 `MaybeLocal<Value>`。
   - 执行失败时返回空；若存在 `TryCatch`，捕获异常并清理 pending exception。
3. **对应测试**
   - 已覆盖：`ScriptRunSucceedsWithFrontendCompiler`
   - 已覆盖：`TryCatchCapturesCompileFailureWithoutFrontendCompiler`

## 7. TryCatch API

### 7.1 `TryCatch::TryCatch(Isolate*)` / `~TryCatch()`

1. **预期行为**
   - 构造入栈，析构出栈，支持嵌套链。
2. **失败语义**
   - 当前无返回值；空 Isolate 路径行为不应被业务依赖。
3. **对应测试**
   - 已覆盖：前后端脚本用例均建立 `TryCatch` 并验证状态。

### 7.2 `TryCatch::HasCaught()`

1. **预期行为**
   - 返回是否已捕获异常。
2. **失败语义**
   - 无异常语义。
3. **对应测试**
   - 已覆盖：前端成功路径断言 false；后端失败路径断言 true。

### 7.3 `TryCatch::Reset()`

1. **预期行为**
   - 清空当前捕获状态。
2. **失败语义**
   - 无异常语义。
3. **对应测试**
   - 已覆盖：后端失败路径 `Reset()` 后再次 `HasCaught()==false`。

### 7.4 `TryCatch::Exception()`

1. **预期行为**
   - 返回已捕获异常对象。
2. **失败语义**
   - 未捕获时返回空 Local。
3. **对应测试**
   - 已覆盖：后端失败路径断言 `Exception()` 非空。

## 8. 函数级审查重点（建议 Gemini 逐项打勾）

1. 生命周期：`New/Dispose` 与注册表释放是否严格配对。
2. 异常契约：`Run` 失败时 `MaybeLocal` 与 `TryCatch` 状态是否一致。
3. 类型语义：`ToString/ToInteger` 对 host/vm 值的分支是否一致且无歧义。
4. 作用域顺序：测试中析构顺序是否稳定（先 scope/catch，再 dispose）。
5. 配置传播：前端链接配置是否可稳定传递到最终可执行目标。

## 9. 建议补测项（当前未完全覆盖）

1. `Saauso::IsInitialized()` 显式断言。
2. `Value::IsString()` / `Value::IsInteger()` 显式断言。
3. `String::Value()` / `Integer::Value()` 显式断言。
4. `Script::Run` 空 context 路径与非法 script 路径。
5. `TryCatch` 嵌套场景（内外层捕获优先级）。
