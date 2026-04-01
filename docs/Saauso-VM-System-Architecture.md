# S.A.A.U.S.O VM 系统架构

本文档为 S.A.A.U.S.O 的系统架构指南，用于介绍解释模块分层、依赖边界、错误传播约定、关键执行协议与核心子系统（GC、解释器、模块系统、前端等）的职责分配，并提供实现索引。

## 1. 分层与依赖方向（V8-like）

本项目的架构设计深度借鉴了 V8 和 HotSpot。

- **目标**：让“对象表示/运行时语义/执行引擎/系统装配”各司其职，避免 `src/objects` 因承载高层语义而膨胀，并把变化点（语义/报错/协议）收敛在更合适的层。
- **utils (`src/utils/`)**：纯工具层；严禁依赖 VM 上层能力（对象/堆/句柄/解释器/运行时/模块系统等）。
- **handles (`src/handles/`)**：GC 安全持有与跨 `HandleScope` 约束；可依赖 `execution` 的 `Isolate::Current()` 绑定模型（现有实现如此）。
- **objects (`src/objects/`)**：对象表示层（布局/字段/GC iterate/instance_size）+ vtable slot 的调度骨架；`py-xxx.cc` 实体类优先保持为“数据容器 + 原语操作”，避免把跨对象的复杂语义流程塞进实体类。
- **execution (`src/execution/`)**：运行时容器与门面层；持有 Isolate/ExceptionState，并通过 `Execution`（AllStatic）收敛“执行/调用”入口。
- **builtins (`src/builtins/`)**：Python API 外壳层；负责参数解析与错误文案；尽量薄，复杂流程优先调用 runtime helper。
- **runtime (`src/runtime/`)**：跨类型语义层；承载可复用流程（iterable unpack、`str(x)`、magic method invoke、建类/反射等）。
- **interpreter (`src/interpreter/`)**：执行层；依赖 objects/runtime/builtins；负责字节码调度、调用协议与栈帧，并消费 Isolate::ExceptionState 做异常展开。

### 1.1. Architecture Boundaries（必须遵守）

为了确保依赖方向单向，避免循环依赖与职责漂移，并为未来引入其他执行器（AOT/JIT）保留演进空间，在 S.A.A.U.S.O 演进过程中，需要始终严格遵守架构边界规范。

具体的规范细则与可执行约束清单，请阅读：
- [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)

## 2. 对象系统 (`src/objects`)

对象系统的设计与实现索引：
- [Object-System.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Object-System.md)

对象系统的强约束（slot 契约、cast like/exact、MarkWord/Tagged/Smi 等）：
- [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)

## 3. 内存管理 (`src/heap`)

内存管理（GC/roots/write barrier/堆空间现状）：
- [GC-and-Heap.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/GC-and-Heap.md)

## 4. 执行与隔离 (`src/execution`)

执行与隔离（Isolate/Current/Locker/初始化销毁流程）：
- [Isolate-Lifecycle-and-Threading.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Isolate-Lifecycle-and-Threading.md)

## 5. Runtime helpers (`src/runtime`)

- **builtins（内建函数）**: 实现在 `src/builtins/`，并由 `BuiltinBootstrapper` 在 `Isolate` 初始化阶段构建后挂到 `isolate->builtins()`（而非放在 `src/runtime/` 或由 `Interpreter` 自身持有）。
- **runtime**: 可复用的运行时 helper（例如 unpack iterable、扩展 list 等）。上层类型实现优先复用此处 helper，减少重复迭代/拆包逻辑。
- **exec 执行入口**：`Runtime_ExecutePyCodeObject/Runtime_ExecutePythonSourceCode` 负责在指定 `globals/locals` 字典中执行代码；当 `globals` 缺少 `__builtins__` 时会自动注入当前 `Isolate` 持有的 builtins（对齐 CPython 行为）。
- **StringTable**: 常用字符串常量池（通常在 `kMetaSpace` 分配字符串对象，避免被 GC 移动）。
  - **文件组织**：按“通用语义 + 对象域 helper”拆分为 `runtime-*.{h,cc}`；除 `truthiness/iterable/intrinsics/conversions/reflection/exec/string` 等通用模块外，当前还稳定包含 `runtime-py-dict/list/tuple/string/function/smi/type-object` 等对象域入口，各 header 只暴露本领域的对外接口，避免出现“万能头文件”导致的编译期耦合。
  - **推荐放置规则**：
    - 字符串相关高层语义（split/join/`str(x)` 转换）优先放在 `runtime-py-string.cc`。
    - 容器与对象族的高层 Python 语义优先放在对应 `runtime-py-*.cc`，避免把跨类型流程重新塞回 `py-xxx.cc` 实体类。
    - iterable 通用语义放在 `runtime-iterable.cc`。
    - 反射/MRO/property find/magic method invoke 放在 `runtime-reflection.cc`。
    - exec/source/code 执行入口放在 `runtime-exec.cc`。

## 6. 字节码解释器 (`src/interpreter`)

解释器实现细节：
- [Interpreter.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Interpreter.md)

## 7. 代码加载 / 前端 (`src/code`)

- **Compiler**：编译入口，提供 `CompileSource/CompilePyc` 等 API。`CompileSource` 的默认实现为“嵌入式 CPython312 编译成 pyc bytes，再解析为 `PyCodeObject`”。
- **cpython312-pyc-file-parser**：解析 CPython 3.12 `.pyc` 并构建 `PyCodeObject`。
- **cpython312-pyc-compiler（可选）**：通过嵌入式 CPython 3.12 生成 `.pyc`（对应 GN 目标 `saauso_cpython312_compiler`）。
- **BinaryFileReader**：二进制读取工具，位于 `src/utils/`，供 pyc parser 等模块复用。

## 8. 模块系统（import，`src/modules`）

模块系统（import）：
- [Module-System.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Module-System.md)

## 9. Embedder API

Embedder API 已有独立文档体系，请阅读：
- [Saauso-Embedder-API-User-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Embedder-API-User-Guide.md)
- [Embedder-API-Architecture-Design.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Embedder-API-Architecture-Design.md)
