# 执行与隔离：Isolate 生命周期与并发模型

本文档收口 S.A.A.U.S.O 的 Isolate 绑定模型、并发访问控制、初始化/销毁流程与全局运行时生命周期（`Saauso::Initialize/Dispose`）的职责与索引。

## 1. Isolate（`src/execution`）

- **Isolate**：独立的虚拟机实例容器，封装堆 (Heap)、对象工厂 (Factory)、句柄作用域实现 (HandleScopeImplementer)、解释器 (Interpreter)、模块管理器 (ModuleManager)、字符串表 (StringTable)、各内建 `Klass` 指针，以及全局单例（`None/True/False`）。
- **Current 绑定模型**：`Isolate::Current()` 通过 `thread_local` 保存当前线程绑定的 Isolate；进入/退出必须使用 `Isolate::Scope`（或显式 `Enter/Exit`），禁止手动设置 Current。
- **多线程访问控制**：`Isolate::Locker` 基于递归互斥锁保证同一时刻仅一个线程访问某个 Isolate（Isolate 级别的“GIL”语义）。
- **关键不变量**：
  - `Isolate::Scope` 的生命周期必须完全被 `Isolate::Locker` 覆盖（若使用多线程访问控制）。
  - `Isolate::Dispose()` 的前置条件是 `entry_count_ == 0`（必须先正确退出所有 scope）。

## 2. 初始化流程（关键）

- 预初始化所有 `Klass`：调用 `PreInitialize()`（完成 vtable 初始填充、把 Klass 注册到 `klass_list_` 等）。
- 初始化 `StringTable`。
- 创建全局单例 `None/True/False`（这些对象需要在大量初始化逻辑之前就可用）。
- 正式初始化所有 `Klass`：`Initialize()`（常见动作：创建类字典、C3/MRO、绑定 type object 等）。
- 初始化 `Interpreter`（提供字节码执行器与调用算法实现，真正的对外执行入口统一由 execution 门面承接）。
- 初始化 `ModuleManager`（负责 `sys.modules/sys.path` 与 import 入口）。
- 构建并挂载 `builtins` 字典（`BuiltinBootstrapper`）。

## 3. 销毁流程（关键）

- 先 `Finalize()` 所有 `Klass`（反向释放与清理元数据）。
- 再依次销毁 `StringTable`、`Interpreter`、`ModuleManager`、`HandleScopeImplementer`、最后销毁 `Heap` 与 `mutex`，并清空 `klass_list_` 与 `None/True/False` 引用（当前实现不保证严格与初始化逆序一致）。

## 4. 全局运行时初始化（`src/init`）

- `saauso::Saauso::{Initialize,Dispose}` 位于 `src/init/`，用于嵌入式 CPython312 编译器前端的 Setup/TearDown。
