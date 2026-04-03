# S.A.A.U.S.O 老生代 GC 与晋升机制开发计划

本文档给出一份正式的开发计划，用于为 S.A.A.U.S.O 实现“仿早期 V8”的老生代空间、老生代垃圾回收算法与新生代晋升机制，并作为后续代码实现、评审与测试验收的统一依据。

## 1. 文档目的

- 为老生代堆空间、major GC 与 promotion 建立清晰的分阶段落地路径。
- 在不破坏现有 MVP 可运行性的前提下，逐步把当前“仅新生代 Scavenge”的实现演进为“简化版早期 V8 风格分代堆”。
- 为外部评审者提供明确的设计边界、风险清单、验收标准与里程碑。

## 2. 当前现状与判断

### 2.1 当前已具备的基础

- 堆已经预留了 `NewSpace + OldSpace + MetaSpace` 的连续地址布局。
- 新生代已经具备可工作的 semi-space Scavenge 主路径。
- 对象系统已经具备统一对象头、`Klass::Iterate` 与 `ObjectVisitor` 精确遍历机制。
- 根集合扫描路径已经覆盖 `Isolate`、`HandleScopeImplementer`、`Interpreter`、`ModuleManager`、`ExceptionState` 等核心持有方。
- `WRITE_BARRIER`、`RecordWrite()` 与 remembered set 已有雏形，说明接口形态已经开始预留。

### 2.2 当前尚不成熟的部分

- `OldSpace` 仍未实现真实分配与地址判定。
- remembered set / write barrier 目前仍处于禁用状态，minor GC 尚未真正支持跨代引用。
- 还没有对象年龄、晋升阈值、promotion failure 处理与 to-old evacuation 逻辑。
- `MarkWord` 仅支持 klass / forwarding-address 语义，不足以直接承载完整 major GC 的标记状态。
- 仓库内部仍存在较多直接字段写入与 `Tagged<T>` 接近裸指针的使用方式，这对 moving major GC 的工程风险较高。

### 2.3 总体判断

当前仓库**不适合直接一次性实现完整的“老生代空间 + mark-sweep + mark-compact + 晋升机制”**。  
当前仓库**适合按阶段推进以下目标**：

1. 先实现可工作的 `OldSpace`。
2. 启用写屏障与 remembered set。
3. 实现新生代到老生代的 promotion。
4. 为老生代先实现稳定的 non-moving `mark-sweep`。
5. 在 major GC 稳定后，再评估 `mark-compact` 是否值得进入第二阶段。

## 3. 目标架构

### 3.1 架构定位

本计划采用“**仿早期 V8，但做工程化简化**”的路线：

- 保留当前 `NewSpace` 的 semi-space copying minor GC。
- 新增一个统一的 `OldSpace`，而不是一次性引入早期 V8 那种更细的 pointer/data/code/map space 拆分。
- 使用 remembered set + write barrier 维护 old-to-new 跨代引用。
- promotion 以“对象年龄 + survivor 压力”双因素驱动。
- 老生代第一阶段使用 `mark-sweep`，第二阶段再评估是否加入 `mark-compact`。

### 3.2 不追求的目标

以下能力不属于本计划第一阶段范围：

- 多老生代子空间拆分。
- 并发/并行 GC。
- 增量标记。
- card table 优化。
- 精细化内存统计、profiler、heap snapshot。
- 兼容所有未来对象种类的最终状态设计。

## 4. 设计原则

### 4.1 正确性优先

- 第一优先级是“不破坏当前 minor GC 正确性”。
- 一切 promotion 与 old-gen GC 设计都必须先保证 root/slot 更新正确。

### 4.2 先非移动，后移动

- 先让 major GC 以 non-moving `mark-sweep` 方式稳定工作。
- 在 major root 遍历、写屏障、promotion、free list 都稳定后，再考虑 `mark-compact`。

### 4.3 最小侵入式演进

- 优先重用现有 `Heap`、`ObjectVisitor`、`Klass::Iterate`、`Handle`/`Global` 机制。
- 尽量避免为了追求“更像 V8”而在第一阶段引入超过项目规模所需的复杂度。

### 4.4 写入路径必须收口

- 任何可能写入 heap slot 的路径，都必须统一走 barrier-aware setter 或等价封装。
- 不允许 promotion 上线后仍存在大量绕过 write barrier 的直接字段赋值。

## 5. 范围定义

### 5.1 第一阶段必须完成

- `OldSpace` 可用。
- remembered set / write barrier 可用。
- promotion 可用。
- old generation `mark-sweep` 可用。
- major/minor GC 都有单元测试与回归测试覆盖。

### 5.2 第二阶段可选完成

- `mark-compact`。
- promotion policy 调优。
- remembered set 从线性 slot buffer 过渡到 card table。

## 6. 关键设计决策

### 6.1 OldSpace 形态

第一阶段采用单一 `OldSpace`：

- 一个地址连续、对象稳定驻留的老生代空间。
- 分配器优先采用 free list + fallback 线性尾部分配的简化方案。
- major GC 结束后把未标记对象回收为 free blocks。

理由：

- 与当前对象系统更匹配。
- 对现有 `Tagged<T>` 使用方式更安全。
- 可以更早把 promotion 路径跑通。

### 6.2 Promotion 策略

采用“双条件晋升”：

- 条件一：对象在 Survivor 中达到年龄阈值。
- 条件二：Survivor 空间压力过大，无法继续安全复制时，允许提前晋升。

建议的起始策略：

- 默认年龄阈值为 `2` 或 `3` 次 Scavenge 存活。
- 当 Survivor 剩余空间不足以容纳待复制对象时，直接转入 `OldSpace`。

### 6.3 Major GC 算法

第一阶段采用 `mark-sweep`：

- 标记阶段：从 roots 与 old-to-old / old-to-meta / meta-to-old 可达图开始标记。
- 清扫阶段：回收未标记对象，形成 free list。
- 对象地址在整个 major GC 过程中保持不变。

第二阶段候选算法为 `mark-compact`：

- 只在 major GC 与地址稳定性问题都充分澄清后进入开发。
- 不作为第一阶段里程碑的阻塞项。

### 6.4 标记元数据策略

本计划建议优先评审两种方案，并在实现前锁定一种：

- 方案 A：扩展 `MarkWord`，为 major GC 增加 mark bit / age 信息。
- 方案 B：引入 side bitmap / side metadata，保持对象头尽量稳定。

建议优先级：

- 第一优先建议评估 **side metadata**。
- 原因是当前 `MarkWord` 已承载 klass / forwarding-address 复用语义，继续扩展对象头会提高 minor GC 与 major GC 头部状态切换复杂度。

## 7. 分阶段实施计划

### Phase 0：设计冻结与代码审计

目标：

- 锁定第一阶段只做 `OldSpace + promotion + mark-sweep`。
- 审计所有潜在 heap slot 写入点。
- 审计所有依赖对象地址稳定性的语义点。

任务：

- 盘点所有对象字段 setter、容器元素写入、工厂初始化路径。
- 标记所有当前绕过 `WRITE_BARRIER` 的直接赋值点。
- 盘点 `repr/hash/identity` 中对对象地址的使用点。
- 梳理 major GC 必须遍历的 native roots 是否已经全部集中到 `Heap::IterateRoots()` 或可扩展入口中。

产出：

- 写入路径清单。
- 地址语义清单。
- GC 元数据设计决议。

验收标准：

- 能明确回答“哪些写入必须改造”“哪些地址语义会阻碍 compaction”“major roots 是否存在盲区”。

### Phase 1：落地 OldSpace 基础能力

目标：

- 实现真实可用的 `OldSpace`。

任务：

- 完成 `OldSpace::Setup/AllocateRaw/Contains/TearDown`。
- 为 `Heap` 增加 `InOldSpace()` 等判定入口。
- 定义老生代分配失败时的 GC 触发策略。
- 为 OldSpace 添加基础 free block 表示与调试断言。

产出：

- 可分配、可判定地址归属、可供 promotion 使用的 `OldSpace`。

验收标准：

- 可以稳定地向 `kOldSpace` 分配对象。
- `OldSpace::Contains()` 正确。
- 基础单元测试覆盖分配、边界、OOM 与 free block 行为。

### Phase 2：启用 Write Barrier 与 Remembered Set

目标：

- minor GC 正式支持 old-to-new 跨代引用。

任务：

- 打开 `WRITE_BARRIER` 主路径。
- 让所有 heap slot 写入统一走 barrier-aware 路径。
- 修复直接字段赋值绕过屏障的问题。
- 启用 `Heap::IterateRememberedSet()`，并在 minor GC 中参与扫描。
- 验证 remembered set 中失效记录的清理逻辑。

产出：

- 可工作的 old-to-new 记忆集机制。

验收标准：

- 老生代对象引用新生代对象后，minor GC 不会错误回收该 young 对象。
- 记忆集在对象晋升、slot 重写、对象死亡后不会无限膨胀。

### Phase 3：实现 Promotion

目标：

- 新生代对象在满足条件时晋升到老生代。

任务：

- 设计并实现对象年龄记录机制。
- 在 Scavenge 复制流程中增加 promotion policy。
- 支持“按年龄晋升”与“按 survivor 压力提前晋升”。
- 处理 promotion failure：如果 old-space 分配失败，则触发 major GC；若仍失败，进入 OOM。
- 确保晋升后 roots、handles、remembered set 均保持正确。

产出：

- 可工作的对象晋升路径。

验收标准：

- 对象经过多次 minor GC 后能进入 old space。
- old object 持有的 young object 在 minor GC 下保持可达。
- promotion failure 行为可预测、可测试。

### Phase 4：实现 Old Generation Mark-Sweep

目标：

- 老生代具备独立 major GC 能力。

任务：

- 设计并实现 mark phase。
- 设计并实现 sweep phase。
- 建立 old-space free list 复用机制。
- 确定 major GC 触发条件。
- 保证 major GC 期间禁止不安全分配，必要时复用 `DisallowHeapAllocation` 约束。

产出：

- 可工作的 old-gen `mark-sweep`。

验收标准：

- 老生代垃圾对象可以被回收并复用空间。
- 多轮 major GC 后堆状态稳定，不出现对象损坏、悬挂引用与 free list 破坏。
- GC 单测与 sanitizer 回归通过。

### Phase 5：补齐系统级测试与回归基线

目标：

- 为分代堆建立长期可维护的测试护栏。

任务：

- 为 young-only、promotion、old-to-new、major GC、OOM、异常路径建立测试矩阵。
- 扩充 handles、interpreter、module manager、exception state 在 major GC 下的回归测试。
- 增加长链对象图、自引用容器、循环引用、模块缓存对象的回归场景。

产出：

- 分代 GC 回归基线。

验收标准：

- 新旧测试全部通过。
- sanitizer 构建下不出现新的 UAF / 越界 / double free。

### Phase 6：评估 Mark-Compact 是否进入第二阶段

目标：

- 只有在 major GC 与写入路径完全稳定后，再决定是否进入 compaction。

进入条件：

- 所有 heap slot 写入路径已收口。
- 地址相关语义已完成审计。
- major roots 与 native roots 没有盲区。
- `mark-sweep` 已具备稳定回归基线。

若进入实现，第二阶段任务包括：

- 设计 compaction forwarding / relocation 方案。
- 扩展 old-space object relocation 流程。
- 更新所有 old-space roots / slots。
- 评估 compaction 后 `repr/hash/identity` 语义是否需要修改或明确约束。

## 8. 风险清单

### 8.1 正确性风险

- 写屏障遗漏导致 young 对象被错误回收。
- promotion 后 remembered set 未正确维护。
- major roots 覆盖不完整导致 old object 被误回收。

### 8.2 架构风险

- `MarkWord` 设计不当会使 minor/major GC 状态耦合过深。
- 为了做 compaction 过早引入太多头部状态或 relocation 复杂度。

### 8.3 工程风险

- 直接字段赋值分散在对象实现中，改造量可能大于预期。
- `Tagged<T>` 的使用若跨 safepoint 保存旧地址，moving GC 风险显著上升。

### 8.4 语义风险

- 目前已有逻辑把对象地址用于 `repr` 和部分 `hash`；若未来实施 mark-compact，需要明确这些语义是否允许变化。

## 9. 建议的实现顺序

推荐严格按以下顺序推进：

1. 审计并冻结设计。
2. 实现 OldSpace。
3. 启用 write barrier 与 remembered set。
4. 实现 promotion。
5. 实现 mark-sweep。
6. 建立测试矩阵。
7. 再决定是否进入 mark-compact。

不建议的顺序：

- 不要在 remembered set 未启用前实现 promotion。
- 不要在 mark-sweep 未稳定前直接实现 mark-compact。
- 不要在写入路径未收口前尝试 moving major GC。

## 10. 里程碑与交付物

### M1：OldSpace Ready

- 交付物：可分配的 OldSpace、相关单测、Heap 地址判定辅助函数。

### M2：Inter-generational Safety Ready

- 交付物：write barrier、remembered set、跨代引用回归测试。

### M3：Promotion Ready

- 交付物：年龄记录、晋升策略、promotion failure 行为定义与测试。

### M4：Major GC Ready

- 交付物：old-gen mark-sweep、free list、major GC 触发策略与测试。

### M5：Review for Compaction

- 交付物：mark-compact 可行性审查报告，而不是默认直接实现。

## 11. 测试与验收要求

### 11.1 必做单元测试

- OldSpace 分配/Contains/free list 行为。
- remembered set 增删改查与失效记录清理。
- young object 晋升到 old space。
- old-to-new 引用在 minor GC 下保持正确。
- major GC 回收垃圾 old object。
- major GC 后对象图与 handles 仍正确。

### 11.2 必做系统回归

- 模块系统缓存对象在多轮 GC 后仍可用。
- 解释器栈帧中对象在 minor/major GC 后都可安全访问。
- 异常对象、`sys.modules`、全局 handle 在多轮 GC 下稳定。
- ASAN/UBSAN 构建下运行关键回归集。

### 11.3 验收门槛

- 能编译。
- 能链接。
- 能稳定通过目标单测。
- 不引入新的 sanitizer 报告。
- 不破坏当前仅新生代场景的既有行为。

## 12. 建议的评审问题

建议外部评审者重点审查以下问题：

1. 第一阶段是否应只做 `mark-sweep`，明确推迟 `mark-compact`。
2. `MarkWord` 与 side metadata 方案应如何取舍。
3. promotion 年龄阈值与 survivor 压力阈值是否合理。
4. OldSpace 分配器第一版应采用何种 free list 组织方式。
5. 当前仓库中哪些地址语义会阻碍未来 compaction。
6. major roots 是否还有潜在遗漏。
7. remembered set 是否应在第一版维持线性 slot buffer，而不是立即上 card table。

## 13. 最终建议

本计划建议把“仿早期 V8”的实现目标解释为：

- 在架构思想上借鉴早期 V8 的**分代堆、semi-space young、promotion、remembered set、major GC**。
- 在工程实现上采用**更适合 S.A.A.U.S.O 当前规模的简化版本**。

因此，推荐的正式路线是：

- **第一阶段：OldSpace + write barrier + remembered set + promotion + mark-sweep**
- **第二阶段：在通过专项评审后再决定是否实现 mark-compact**

这是当前最符合仓库现状、风险可控、且最容易被测试和评审接受的方案。
