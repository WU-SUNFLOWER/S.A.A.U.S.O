# S.A.A.U.S.O Embedder API (MVP) 架构与开发方案

## 1. 背景与目标

S.A.A.U.S.O 的核心项目定位之一是**可作为轻量级脚本引擎提供给游戏引擎、办公软件等 Embedder 使用，且便于嵌入方开发者进行自由修改或功能裁剪**。
随着当前虚拟机核心架构（如 `Isolate`、`HandleScope`、`MaybeHandle`、`Factory`、`ExceptionState` 等）逐渐趋于稳定并与 V8 架构高度对齐，当前已具备构建标准 Embedder API 的基础。

本方案旨在制定一套 MVP（最小可行性产品）版本的 V8-like 风格 Embedder API 开发蓝图，为外部 C++ 程序提供安全、隔离、易用的 Python 脚本嵌入能力。

***

## 2. MVP 阶段 Embedder API 设计

在 MVP 阶段，我们将通过 `include/saauso.h` 等公开头文件向外暴露以下核心 API，屏蔽底层 `Tagged<T>`、`Klass` 等内部细节。

### 2.1 基础设施与生命周期管理

* **`saauso::Isolate`**: 虚拟机的物理实例。嵌入方通过 `saauso::Isolate::New()` 创建，拥有独立的堆内存和执行栈。提供完全的运行实例隔离。

* **`saauso::HandleScope`**: RAII 风格的句柄作用域。嵌入方在 C++ 栈上声明，用于自动管理 C++ 层持有的 Python 对象引用（`Local<T>`），防止垃圾回收（GC）误回收或内存泄漏。

* **`saauso::Local<T>`**: 暴露给嵌入方的安全对象指针（对应内部的 `Handle<T>`）。

* **`saauso::Context`** *(可选/建议)*: 运行上下文，封装全局变量环境（如 `__main__` 模块的 `__dict__`），实现同一个 Isolate 下的隔离执行环境。

### 2.2 数据类型映射 (Value Hierarchy)

提供一个从 `saauso::Value` 派生的 C++ 类层次结构，安全包装内部的 `Tagged<PyObject>`：

* **`saauso::Value`**: 所有 Python 对象的基类，提供类型判断接口（如 `IsString()`, `IsInt()`, `IsDict()`）。

* **基本类型**: `saauso::String`, `saauso::Integer`, `saauso::Float`, `saauso::Boolean`。提供与 C++ 原生类型（`int`, `double`, `std::string`）的互转。

* **容器类型**:

  * `saauso::Object`: 提供 `Set(key, value)`, `Get(key)` 用于属性访问。

  * `saauso::List`, `saauso::Dict`, `saauso::Tuple`: 提供基础的容器操作（如 `Length()`, `Get(index)`）。

* **`saauso::Function`**: 包装可调用对象，提供 `Call(receiver, argc, argv)` 接口供 C++ 主动调用 Python 函数。

### 2.3 脚本编译与执行

* **`saauso::Script`**:

  * `Script::Compile(source_string)`: 将 Python 源码字符串编译为字节码对象。

  * `Script::Run(context)`: 在指定的上下文中执行字节码并返回结果（`Local<Value>`）。

### 2.4 异常处理

* **`saauso::TryCatch`**: RAII 风格的异常捕获器。当 C++ 代码调用 `Script::Run` 或 `Function::Call` 时，如果虚拟机内部抛出异常，会被拦截到最近的 `TryCatch` 块中。嵌入方可通过 `HasCaught()` 和 `Exception()` 获取异常信息并进行 C++ 层的处理，防止宿主程序崩溃。

### 2.5 C++ 扩展注入 (Interop)

* **Function Callback**: 允许嵌入方注册 C++ 函数给 Python 调用。通过签名（如 `void MyCppFunc(const saauso::FunctionCallbackInfo& info)`）将游戏引擎或宿主 API 暴露给脚本层。

***

## 3. 虚拟机内部架构准备工作

为了支撑上述对外的干净 API，内部代码需要进行严格的边界划分和桥接准备：

### 3.1 目录结构与可见性隔离

* **机制**：建立严格的 `include/`（公开 API）与 `src/`（内部实现）分离机制。

* **规则**：`include/saauso.h` 及其包含的文件**绝对不能**引入 `src/objects/`、`src/interpreter/`、`src/heap/` 等内部头文件。对外完全隐藏 `Tagged<T>`、`Klass`、`Factory` 的实现。

### 3.2 API 桥接层设计 (API Wrapper)

* **机制**：在 `src/api/api.cc` 中实现桥接逻辑。

* **实现**：利用 C++ 的强转（`reinterpret_cast`）将内部的 `Handle<T>` 零成本转换为公开的 `Local<T>`。例如，`saauso::String::New(isolate, "hello")` 内部会路由至 `isolate->factory()->NewStringFromAscii("hello")`。

### 3.3 异常机制 (TryCatch) 与 ExceptionState 打通

* **现状**：异常状态当前保存在 `Isolate::ExceptionState` 中。

* **改造**：在 `Isolate` 中维护一个 `TryCatch` 链表。当内部抛出异常时，如果存在外部 C++ 的 `TryCatch` 块，则挂起异常，不直接触发内部的 crash 或 print。当内部的 `MaybeHandle` 返回 `kNullMaybe` 时，控制权平滑交还给 C++ 嵌入层，由外部 `TryCatch` 消费异常。

### 3.4 Context (全局环境) 的显式化

* **现状**：目前执行可能隐式依赖单一的全局 Bootstrapper 或固定模块。

* **改造**：明确“全局环境”的边界。执行 `Script::Run` 时，必须能传入或获取当前线程的 `Context`，以便正确解析全局变量和内置模块。

***

## 4. 详细开发 Roadmap

建议将整个 Embedder API 的开发分为五个阶段推进，确保逐步交付、步步可测：

### Phase 0: 内部架构校验与微调 (预计 0.5 周)

* **目标**：确认当前虚拟机内部组件对 Embedder API 的支撑度，并进行最小侵入式的微调。

* **调研结论**：目前 S.A.A.U.S.O 的核心架构（`Isolate`、`Handle`、`Execution` 门面、`Runtime_Execute*` 家族）**极其成熟，无需进行任何伤筋动骨的底层重构**。相关机制已完美就绪，仅需在 API 边界进行桥接。

* **核心任务**：

  1. **Context 执行原语确认**：无需修改底层的 `Interpreter::Run`。未来 `saauso::Script::Run(context)` 将直接复用 `src/runtime/runtime-exec.cc` 中的 `Runtime_ExecutePyCodeObject`，该函数已原生支持传入自定义的 `globals` 字典（即 Context）并安全创建临时 `PyFunction` 执行，避免了环境污染。
  2. **TryCatch 机制复用**：确认内部 `ExceptionState::Clear()` 已完全可用。`saauso::TryCatch` 仅需在 `api.cc` 层面实现：检查 `Isolate::HasPendingException()` -> 提取异常信息 -> 调用 `Clear()` 恢复虚拟机状态，不涉及内部异常抛出链路的修改。
  3. **构建系统 (BUILD.gn) 调整**：将现有的 `saauso_core` source\_set 进一步明确为供 Embedder 链接的库目标，准备将未来的 `src/api/api.cc` 统一纳入其中，并保持 CLI 入口 (`main.cc`) 作为独立可执行文件 (`vm`)。

### Phase 1: 基础设施与 API 骨架搭建 (预计 1-2 周)

* **目标**：建立 `include/` 公开头文件体系，跑通第一个 Embedder 实例。

* **核心任务**：

  1. 定义 `include/saauso.h`（包含 `Isolate`, `HandleScope`, `Local<T>`, `Value` 基类的空壳/声明）。
  2. 实现 `src/api/api.cc`，完成 `Isolate::New()`, `Isolate::Dispose()` 的内部绑定。
  3. 完成 `HandleScope` 与内部 `HandleScopeImplementer` 的联动逻辑。

* **测试验证**：编写 `test/embedder/test-isolate.cc`，能够成功初始化和销毁虚拟机，利用 AddressSanitizer (ASan) 或 Valgrind 验证无内存泄漏。

### Phase 2: 核心类型与脚本执行 MVP (预计 2-3 周)

* **目标**：Embedder 能够从 C++ 注入字符串/数字，并编译执行一段简单的 Python 脚本。

* **核心任务**：

  1. 实现基本类型 `saauso::String`, `saauso::Integer` 的 `New()` 创建和 `Value()` 数据提取。
  2. 封装 `saauso::Script::Compile` (对接内部 `Compiler::CompileSource`)。
  3. 封装 `saauso::Script::Run` (对接 `Interpreter::Run` 或 `Execution::Call`)。
  4. 实现 `saauso::TryCatch` 机制，接管 `Isolate::ExceptionState`。

* **测试验证**：编写 `hello_world.cc` 示例程序，在 C++ 中执行 `print("Hello from S.A.A.U.S.O Embedder!")`；验证 `TryCatch` 能否成功捕获包含语法错误或运行时错误的恶意脚本。

### Phase 3: 数据交互与函数互调 (预计 2-3 周)

* **目标**：实现 C++ 与 Python 的深度交互（传参、获取返回值、C++ API 注入）。

* **核心任务**：

  1. 实现 `saauso::Object`, `saauso::List`, `saauso::Dict`，封装内部的 `PyDict::PutMaybe` 等 Fallible API。
  2. 实现 `saauso::Function` 及其 `Call` 方法。
  3. 实现 `FunctionCallbackInfo` 机制，允许把 C++ 函数包装成 `PyNativeFunction` 注册到 Python 模块中。

* **测试验证**：编写**游戏引擎模拟测试**：C++ 暴露 `GetPlayerHealth()` 等函数给 Python；Python 脚本调用后根据血量返回状态；C++ 接收返回的状态并更新外部变量。

### Phase 4: 健壮性增强与文档化 (预计 1 周)

* **目标**：达到可正式发布给第三方使用的标准。

* **核心任务**：

  1. 补充 Embedder API 的 Doxygen 格式头文件注释。
  2. 增加 API 层的边界参数校验（如 `Local<T>` 判空，多线程 `Isolate` 访问检查）。
  3. 在 `examples/` 目录下提供标准的使用范例和 CMake 引用示例。

* **测试验证**：进行极端的 GC 压力测试，验证 `HandleScope` 边界和 GC 回收阶段的稳定性。

