# 对象创建 Factory 化与 GC 安全治理计划（阶段 0-2）

## 背景

当前 `src/objects/**` 中大量对象工厂/构造入口直接调用 `Heap::Allocate/AllocateRaw` 并自行完成字段初始化与关联对象分配。该模式带来两个长期问题：

- Heap 分配 API/策略发生调整时，修改面过大且难以系统性回归。
- 对象初始化缺乏制度化约束，容易出现“先分配（可能触发 GC）再初始化指针字段”的隐蔽崩溃风险。

本计划参考 V8 的 Factory 抽象，在 `Isolate` 下引入 `Factory` 单例，并引入 `DisallowHeapAllocation` 机制，逐步收敛对象创建路径并固化 GC 安全不变量。

## 目标

- 抽象边界：VM 内部对象创建逐步不再直接依赖 `Heap` 分配细节；对象层更接近“内建对象/数据容器”。
- GC 安全：在对象处于“GC 可见但字段未安全初始化”阶段时，建立 fail-fast 机制避免隐蔽崩溃。
- 渐进治理：允许阶段性共存（旧路径保留），每一步都可全量回归单元测试。

## 核心不变量（阶段 0 固化）

### I1：分配边界（治理目标态）

- 目标态下，`src/objects/**` 不直接调用 `Heap::Allocate/AllocateRaw`。
- `Factory` 作为 “handle-based allocation interface”，统一封装分配语义，并逐步承载内建对象创建 API（例如 `NewPyDict/NewFixedArray/NewPyStringLike/...`）。

### I2：初始化安全（制度化约束）

- 当一个新分配出的对象已对 GC 可见时，在进行任何可能触发 GC 的操作之前，必须先将其所有指针字段写入可扫描的哨兵值（`Tagged::null()/kNullAddress` 等）并设置必要的元信息（`klass/properties` 等）。
- 在实现上采用组合拳：
  - Factory 统一模板：将“安全初始化”的骨架固化在 Factory 内，减少样板与遗漏面。
  - `DisallowHeapAllocation`：在“对象字段安全初始化阶段”禁止再次发生堆分配（任何分配会 fail-fast），从机制上阻断“先分配关联对象/容器再清字段”的风险。

## 阶段划分与验收

### 阶段 0：约定与边界落地（文档 + 可执行不变量）

交付物：

- 本文档（治理目标、不变量、阶段划分与验收标准）。

验收：

- 文档明确 I1/I2，不引入破坏现有语义的代码改动。

### 阶段 1：引入 Factory 单例（基础设施）

交付物：

- `Factory` 类型与 `Isolate::factory()` 访问器，生命周期由 `Isolate` 管理。
- `Factory` 统一封装：
  - `Allocate<T>(space)` / `AllocateRaw(size, space)`

验收：

- `//test/unittests:ut` 全量通过。
- 现有对象创建路径保持可用（允许旧路径共存）。

### 阶段 2：引入 GC 安全制度（模板 + 禁止分配机制）

交付物：

- `DisallowHeapAllocation`（可嵌套 RAII）：
  - 作用域内禁止任何 `Heap::Allocate*`。
  - 违反约束直接 fail-fast（debug 下断言）。
- 在 Factory 的部分对象创建路径中启用统一模板，确保对象在分配后、任何关联对象分配前完成“字段安全初始化”。

验收：

- `//test/unittests:ut` 全量通过。
- 至少覆盖一组典型对象创建路径（含需要分配关联容器/字典的对象），证明模板 + 禁止分配机制有效。

## 后续阶段（阶段 3+ 概览，仅用于讨论）

### 阶段 3：批量迁移 + 门禁（治理加速期）

交付物：

- 批量迁移：将 `src/objects/**` 中剩余的“直接触达 `heap()->Allocate/AllocateRaw`”的对象创建路径迁入 `Factory`（或至少将分配动作迁入 `Factory`），并在 `Factory` 中延续阶段 2 的统一模板（先安全初始化，再分配关联对象/容器）。
- 门禁：新增可执行的边界检查，确保后续改动不会把 `heap()->Allocate*` 再次引回 `src/objects/**`（允许 `*-klass.*` 作为例外）。

验收：

- `src/objects/**`（排除 `*-klass.*`）不再出现 `->heap()->Allocate*` 或 `->heap()->AllocateRaw` 调用点。
- `//test/unittests:ut` 全量通过，且包含门禁用例。

已完成的阶段 3 迁移清单（第一批）：

- 容器与字符串：`PyList::AllocateListLike`、`PyTuple::AllocateTupleLike`、`PyString::AllocateStringLike`
- 复杂对象：`PyCodeObject::NewInstance` 的分配/安全初始化
- 轻量对象：`Cell::NewInstance`
- 迭代器/视图：`PyListIterator/PyTupleIterator`、`PyDictKeys/Values/Items` 与 `PyDict*Iterator`
- 元对象：`Klass::CreateRawPythonKlass` 的分配/默认字段初始化

门禁实现：

- 新增单元测试扫描 `src/objects/**` 源码文本，若发现非 `*-klass.*` 文件中出现 `->heap()->Allocate*` / `->heap()->AllocateRaw` 则测试失败。
