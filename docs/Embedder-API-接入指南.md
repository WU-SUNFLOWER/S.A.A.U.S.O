# S.A.A.U.S.O Embedder API 接入指南（MVP）

## 1. 这是什么

S.A.A.U.S.O Embedder API 用于把脚本能力嵌入到 C++ 宿主程序中。  
你可以：

1. 在 C++ 创建 `Isolate/Context`
2. 向脚本侧注入宿主回调（`Function::New`）
3. 编译运行脚本（`Script::Compile/Run`）
4. 用 `TryCatch` 捕获错误
5. 在 C++ 与脚本间交换 `String/Integer/Float/Boolean/Object/List/Tuple`

## 2. 最小接入流程

1. 初始化运行时：`Saauso::Initialize()`
2. 创建隔离实例：`Isolate::New()`
3. 进入作用域：`HandleScope`
4. 创建上下文：`Context::New()`
5. 通过 `Context::Set/Get` 或 `Context::Global()` 注入/读取全局变量
6. 如开启前端编译能力，调用 `Script::Compile` + `Run`
7. 用 `TryCatch` 观察失败路径
8. 结束时 `isolate->Dispose()` + `Saauso::Dispose()`

## 3. 常用 API 速览

### 3.1 基础生命周期

1. `Isolate::New/Dispose`
2. `HandleScope`
3. `Context::New/Enter/Exit`
4. `Context::Global`（返回全局字典对应的 `Object`）

### 3.2 值类型

1. `String` / `Integer` / `Float` / `Boolean`
2. `Object`：`Set/Get/CallMethod`
3. `List`：`Length/Push/Set/Get`
4. `Tuple`：`New/Length/Get`

### 3.3 回调与异常

1. `Function::New`（注册宿主回调）
2. `Function::Call`
3. `FunctionCallbackInfo`（读参数、写返回值、`ThrowRuntimeError`）
4. `TryCatch`（统一捕获 API 失败）
5. `Exception::TypeError/RuntimeError`（MVP 工厂接口）
6. `Isolate::ThrowException`（宿主主动注入异常）

## 4. 错误处理规范（必须遵守）

1. 所有可失败 API 都可能返回空 `MaybeLocal<T>`。
2. 只要你关心失败原因，就在调用前创建 `TryCatch`。
3. `MaybeLocal` 失败 + `TryCatch.HasCaught()==true` 才是完整错误闭环。
4. 不要忽略空句柄；必须先判空再 `ToLocalChecked`。

## 5. 一个实战 Demo 在哪里

1. 文件：`examples/embedder/game_engine_demo.cc`
2. 场景：宿主提供 `GetPlayerHealth/SetPlayerStatus`，Python 脚本实现怪物 AI 的 `on_update`。
3. 重点：演示 C++ -> Python -> C++ 双向调用，以及错误捕获路径。

## 6. 当前已知限制（请务必阅读）

### 6.1 内存与 OOM

1. 当前 OOM 不是以可恢复异常对外暴露。
2. 在底层分配失败的极端情况下，进程可能直接终止（例如 `std::exit(1)` 路径）。
3. 结论：MVP 阶段不承诺 OOM 可恢复。

### 6.2 并发与线程

1. `Isolate` 目前按单线程访问模型使用。
2. 不保证同一个 `Isolate` 可被多个线程并发调用 API。
3. 若要并行，请使用多个 `Isolate` 并做好外部线程隔离。

### 6.3 前端编译开关

1. `Script::Compile/Run` 的完整脚本体验依赖 `SAAUSO_ENABLE_CPYTHON_COMPILER`。
2. 在未开启该能力时，`Compile` 失败是预期行为。

### 6.4 兼容性说明

1. 当前为 MVP 迭代期，不承诺稳定二进制 ABI。
2. 升级版本时请重新编译宿主工程。

## 7. 接入建议

1. 先用 `examples/embedder/hello_world.cc` 跑通生命周期。
2. 再跑 `examples/embedder/game_engine_demo.cc` 验证互调能力。
3. 在业务代码里统一封装 `TryCatch` + `MaybeLocal` 判空模板，避免漏判。
4. 对宿主回调参数做显式校验，错误统一走 `ThrowRuntimeError` 或 `Exception` 工厂。
