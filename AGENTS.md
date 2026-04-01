# S.A.A.U.S.O 项目 - AI 助手指南

本文档概述了 S.A.A.U.S.O 项目的架构决策、代码标准和开发工作流。AI 助手需要扮演一位具备 Google C++ Readability 认证的 Senior Software Engineer（Google L5 职级），在生成代码时应参考本文档，以确保与现有代码库保持一致。

## 0. 给 AI 的执行规则（必读）

### 0.1. 执行优先级与冲突处理

- **规则优先级（从高到低）**：
  1. 仓库现有实现与“能编译/能链接/能运行单测”的事实
  2. 本文档（`AGENTS.md`）
  3. 项目代码规范（`docs/Saauso-Coding-Style-Guide.md`）
  4. 通用最佳实践，包括但不限于：
     - SOLID 原则
     - Don't Repeat Yourself 原则
     - The Boy Scout Rule 规则
- **冲突处理流程**：
  1. 先在仓库中搜索同类代码与既有模式（优先保持局部一致性）。
  2. 若仍存在多种可行实现，选择更符合本仓库架构约束、且对现有代码扰动更小的方案。
  3. 若约束仍不明确，优先保证行为正确与可测试性，再逐步收敛风格。
- **用户指令优先**：当用户的最新明确指令与本文档冲突时，以用户指令为准；但应在交付说明中点出偏离点与潜在风险，并尽量把偏离范围限制在最小。

### 0.2. Top 10 必守规则（AI Checklist）

1. 修改前先在仓库中搜索同类实现与既有模式，再决定具体写法（见 0.1）。
2. 禁止在接口上传递 `PyObject*`；对外暴露与内部调用使用 `Tagged<PyObject>` 或 `Handle<PyObject>`（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
3. 栈上持有 GC-able 对象必须使用 `Handle<T>`；跨 `HandleScope` 返回必须使用 `EscapableHandleScope::Escape`（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
4. 分配 `PyObject` 派生对象禁止使用 `new`；必须优先通过 `Isolate::Current()->factory()->NewXxx(...)` 进入统一工厂路径（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
5. 不依赖构造函数写默认值：`Allocate/AllocateRaw` 不清零且不调用构造函数，默认值应在工厂函数中手工写入（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
6. 新增/重写 `Klass::vtable_` 的 slot 时必须显式指向默认实现，或确保所有调用点对 `nullptr` 可处理（见 [Saauso-VM-System-Architecture.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)）。
7. `instance_size` 必须为“不可触发 GC”的纯计算；`iterate` 必须遍历对象内全部 `Tagged<PyObject>` 引用字段（见 [Saauso-VM-System-Architecture.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md) 与 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
8. `src/utils/` 严禁依赖虚拟机上层能力；不确定时先查同目录既有代码并保持依赖方向单向（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
9. 所有内部代码必须位于 `namespace saauso::internal`，并遵循代码风格指南 [Saauso-Coding-Style-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md) 的命名与注释规范（必须包含必要的简体中文注释）。
10. 新增单元测试文件后必须同步加入 `test/unittests/BUILD.gn` 的 `sources` 列表（见 [Saauso-Build-and-Test-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)）。

## 1. 项目概览

S.A.A.U.S.O 是一款高性能 Python 虚拟机，旨在兼容 CPython3.12 字节码。

- **开发者**: WU-SUNFLOWER（Computer Science专业本科大四学生）
- **开发目的**：
- 这是WU-SUNFLOWER的本科毕业设计。
- 他希望通过开发这个项目，在顺利拿到CS专业学士学位的同时，帮助自己提升对编程语言虚拟机的理解和工程实现能力。为深入阅读和学习工业级虚拟机（如HotSpot JVM、V8、CPython）的源代码打下坚实的基础。
- 此外，WU-SUNFLOWER希望能够将本项目写进求职简历的"个人作品"栏目，增强他在未来跳槽时的市场竞争力。
- **项目定位**：
  - 可用于本科计算机专业课程（如《编译原理》《操作系统》等）的演示和教学。（对应毕业设计的教育教学价值）
  - 可作为轻量级脚本引擎提供给游戏引擎、办公软件等 embedder 使用，且便于嵌入方开发者进行自由修改或功能裁剪。（对应毕业设计的实际应用价值）
- **语言**: C++20。
- **构建系统**: GN (Generate Ninja) + Ninja。
- **测试框架**: Google Test。
- **辅助调试工具**: LLDB、AddressSanitizer、UBSanitizer
- **兼容系统平台**：Windows(x64)、Linux(x64)

### 1.1. 当前实现进度（以仓库现状为准）

- **已具备**：
  - 基础对象系统：`PyObject/Klass/VTable`、`PyTypeObject`，以及若干内建类型（`int(Smi)`/`float`/`str`/`list`/`tuple`/`dict`/`bool`/`None`）；同时包含少量“内部基建对象”（例如 `FixedArray`/`Cell`/各类 iterator/view），用于实现语义与性能优化，通常不直接暴露到 Python 层。
  - 句柄系统：`HandleScope` + `HandleScopeImplementer`，以及长期句柄 `Global<T>`（会被 GC 扫描并在 minor GC 后更新）。
  - 堆与 GC：`NewSpace/MetaSpace` 已可用；`OldSpace` 地址段已预留，但分配与回收尚未实现；MVP 仅依赖新生代 scavenge。
  - 字节码解释器：基于 CPython 3.12 字节码模型的执行引擎（computed-goto dispatch）、栈帧与参数绑定；`builtins` 在 `Isolate::Init()` 阶段由 `BuiltinBootstrapper` 统一构建并挂到 `Isolate`（基础 builtins、基础类型名、单例与最小异常类型等）。
  - 对象分配工厂：`Factory` 已成为对象创建主入口，统一封装分配与字段初始化顺序；分配入口带有 `DisallowHeapAllocation` 保护，避免对象初始化未完成时触发嵌套分配。
  - 最小异常对象体系：`Isolate::ExceptionState` + `Runtime_Throw*` 抛错门面；解释器侧注入 `BaseException/Exception/TypeError/RuntimeError/ValueError/IndexError/KeyError/NameError/AttributeError/ZeroDivisionError/ImportError/ModuleNotFoundError/StopIteration` 等最小异常类型对象。
  - 异常处理（解释器侧）：已接入 CPython 3.11+ 的 `co_exceptiontable`（exception table）解析、handler 查找与 zero-cost unwind 主路径；`try/except`、`raise/reraise`、`finally` 等核心异常展开语义已有专项回归测试覆盖。
  - 模块系统（import）：新增 `ModuleManager/ModuleImporter/ModuleLoader/ModuleFinder/ModuleNameResolver/PyModule` 等，解释器接入 `IMPORT_NAME/IMPORT_FROM` 并支持 `level` 相对导入；支持从 `sys.path` / `package.__path__` 加载 `.py/.pyc` 与 package（`__init__.py/__init__.pyc`）；支持 `fromlist` 语义与父子模块绑定（`pkg.sub = <module>`）；内建模块通过注册表机制优先命中（当前含 `sys/math/random/time`，其中 `sys.modules/sys.path` 直接引用 `ModuleManager` 维护对象）。
  - `.pyc` / 源码前端：CPython 3.12 `.pyc` 解析器与（可选）嵌入式 CPython 3.12 编译器前端目标；开启 `saauso_enable_cpython_compiler` 时可直接走源码编译执行链路。
- **尚未重点覆盖/仍在 TODO（非穷尽）**：
  - 老生代回收，以及分代式 GC 所需的 remembered set / write barrier（已实现雏形，但当前通过宏与 root-iterate 路径整体禁用，MVP 不做跨代引用处理）。
  - 更完整的 Python 语义：更完整的 importlib/元路径机制、class/descriptor 语义细节、生成器/协程等。

### 1.2. Embedder API（外部嵌入能力）阅读指引

当任务涉及“把 S.A.A.U.S.O 当脚本引擎嵌入业务系统”时，请优先进入 Embedder 语境，不要直接从解释器内部实现下手。

- **先读接入指南（面向使用者）**：
  - `docs/Embedder-API-接入指南.md`
  - 适用：快速上手、示例代码、错误处理约定、已知限制说明。
- **再读架构设计文档（面向实现者/CR）**：
  - `docs/Embedder-API-架构设计思路.md`
  - 适用：分层边界、生命周期策略、互调链路、异常模型、源码索引入口。
- **代码审查建议顺序**：
  1. 对外头文件（`include/saauso.h` 与 `include/saauso-*.h`）
  2. 桥接层（`src/api/`）
  3. 生命周期/互调关键链路（`src/api/api-*.cc`、`src/runtime/runtime-py-function.cc`、`src/objects/py-function-klass.cc`）
  4. 示例与测试（`samples/`、`test/embedder/`）

## 2. 仓库结构速览

**仓库结构**与**建议阅读路线**已疏解到独立文档：

- [Saauso-Project-Structure-And-Reading-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Project-Structure-And-Reading-Guide.md)

## 3. 架构指南

系统架构、依赖边界与关键调用/错误约定已疏解到独立文档：

- [Saauso-VM-System-Architecture.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)

## 4. 代码规范

代码风格与注释规范，请严格遵守以下文档：

- [Saauso-Coding-Style-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md)

特别地，**当“风格规范”与“能编译/能跑单测”的事实冲突时，以仓库现状与可验证性为最高优先级（见 0.1）。**

## 5. 开发工作流

构建、测试与单测架构：

- [Saauso-Build-and-Test-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)

## 6. 给 AI Agent 的关键实现细节

与 VM 正确性强相关的工程约束和编程范式（Handle/Tagged、对象分配与初始化、GC 遍历、utils 依赖边界、新增内建类型 checklist）：

- [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)

