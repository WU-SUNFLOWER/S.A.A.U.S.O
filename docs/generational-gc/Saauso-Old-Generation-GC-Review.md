# S.A.A.U.S.O 老生代 GC 与晋升机制开发计划 - 技术评审报告

**评审人**：高级虚拟机工程师 (ChatGPT)
**评审对象**：《S.A.A.U.S.O 老生代 GC 与晋升机制开发计划》
**评审结论**：**强烈建议接受并作为后续开发基线。**

---

## 1. 总体结论

这份开发计划是一份**极具工程素养、非常务实**的设计文档。它精准地把握了从“仅有 Scavenge 的 MVP”向“完整分代 GC”演进的工程痛点，没有陷入“大而全”的完美主义陷阱。

计划在架构演进路径上非常稳健，核心的“先非移动（Mark-Sweep），后评估移动（Mark-Compact）”策略，最大程度地规避了 C++ 宿主语言中 `Tagged<T>` 误用带来的毁灭性 Bug。结合团队在后续 Q/A 中确定的关键语义取舍（剥离 `id()` 的内存地址依赖、废弃 `__del__`、严格封装对象字段），该计划具备极高的落地可行性。

---

## 2. 多维度技术审查

### 2.1 架构方向
- **评价**：极佳。采用“简化的分代堆”是非常明智的。统一的 `OldSpace` 足以验证分代假说（Generational Hypothesis），避免了早期 V8 细分 Map/Code/LargeObject/OldData 等空间带来的过早复杂化。

### 2.2 GC 算法选型
- **评价**：合理。第一阶段选择 Non-moving 的 `mark-sweep` 是最正确的决定。在 C++ 实现的 VM 中，Moving GC 对根节点精确性（Exact Rooting）和内部指针的要求极高。先用 Mark-Sweep 把跨代引用、写屏障、晋升机制跑通，是隔离复杂度的最佳手段。

### 2.3 Promotion 设计
- **评价**：经典且有效。“年龄阈值 + Survivor 空间压力”双因素驱动是 HotSpot 和 V8 都在使用的成熟策略，能有效防止 Survivor 被长生命周期对象撑爆导致退化为全量老生代分配。

### 2.4 Write Barrier / Remembered Set 设计
- **评价**：顺序正确。在 Phase 2 强制收口写路径并启用 Write Barrier，是极其关键的前置步骤。计划中提到从“线性 slot buffer”起步，这也是合理的务实选择。

### 2.5 OldSpace 设计
- **评价**：务实。Free list 配合 fallback 线性分配是标准的 Non-moving 空间管理方式。对于包含大量固定大小类型的 Python 对象系统，这种分配器足够高效。

### 2.6 `MarkWord` / Side Metadata 方案取舍
- **评价**：**强烈建议锁定方案 B（Side Metadata / Bitmap）**。`MarkWord` 在 minor GC 中用于存 forwarding pointer 已经很脆弱了。如果强行把 major GC 的 mark bit 和 age 塞进去，会导致对象头状态机异常复杂。现代 VM 设计（V8、ZGC、G1）均全面拥抱 Side Bitmap，它不仅让对象头干净，还能让 Sweep 阶段极其高效（按字操作 Bitmap 寻找连续空闲块）。

### 2.7 Major GC 风险与 Mark-Compact 评估
- **评价**：风险识别准确。最大的工程灾难通常来源于“漏写 Write Barrier”。文档明确把“写入路径收口”放在 Phase 0，非常有前瞻性。原计划对 Mark-Compact 的推迟非常明智，而在明确了后续的架构决策（见第3节）后，Mark-Compact 的最大语义阻碍已被扫除，但作为第二阶段的评估项依然是稳妥的。

### 2.8 测试与验收标准
- **评价**：严谨。明确依赖 ASAN/UBSAN，并把“不破坏当前 minor GC 行为”作为红线，体现了很好的 CI/CD 与工程护栏意识。

---

## 3. 核心架构决策与共识 (Q/A)

针对原计划中存在的几个潜在语义和工程陷阱，评审人与架构师进行了深入探讨，并达成以下重要共识。这些决策极大地降低了 GC 实现的复杂度和未来演进的阻力：

### Q1：Python 中的 `id()` 与 `hash()` 强依赖对象内存地址，如果未来做 Mark-Compact 导致对象移动，如何处理？
- **架构决策**：**引入懒加载的独立 Identifier。** S.A.A.U.S.O 在 MVP 阶段暂不支持 `id()`。在日后版本中，将借鉴 V8 等现代 VM 的做法，为对象引入一套借助随机数生成器懒加载的 identifier（可存放在对象头或 Side Table 中），替代内存地址作为默认的 `id()` 和 `hash()` 结果。
- **评审意见**：**完全赞同。** 这是解耦对象标识与物理内存地址的唯一正确做法，彻底扫除了未来实现 Moving GC（如 Mark-Compact）在语言语义层面的最大障碍。

### Q2：Python 中的 `__del__`（析构器）在 Major GC 中如何处理？它通常会引发对象复活（Resurrection）和复杂的清理队列问题。
- **架构决策**：**不支持 `__del__`，全面拥抱 RAII（`with` 语法）。** 调研表明主流 VM（JS, Java, C#, Dart）均不建议或不支持对象析构回调。S.A.A.U.S.O 决定强制要求业务接入方采用 `with` 进行资源管理。
- **评审意见**：**极其明智的减负。** 终结器（Finalizer）是所有 GC 实现者的梦魇。放弃 `__del__` 使得 VM 的可达性分析和回收逻辑变得清晰且单向，大幅提升了 GC 的性能和稳定性。

### Q3：如何从工程上确保“直接字段赋值绕过屏障”的问题被彻底修复，而不仅仅依靠人工代码审计？
- **架构决策**：**C++ 访问控制强制隔离。** 将所有包含对象裸引用（`Tagged<T>`）的字段设为 `private`。虚拟机内部读操作统一走 public getter；写操作统一走带 write barrier 的 setter。
- **评审意见**：**最佳实践。** 纯靠人工审计必定会漏。利用 C++ 的访问控制机制（Access Modifiers）从编译器层面阻断野蛮写入，是 V8 (`Object::set_...`) 和 HotSpot (`oopDesc::obj_field_put`) 都在采用的标准工程手段。

---

## 4. 评审意见与最终建议

基于上述评估与共识，对后续执行提出以下最终建议：

### 4.1 建议保留的核心设定
1. **全部的 Phase 阶段划分与先后顺序**（Phase 1 到 Phase 5 堪称教科书级别的演进路线）。
2. **渐进式演进**：先 Non-moving (Mark-Sweep)，后评估 Moving (Mark-Compact)。
3. **双条件晋升策略**（Age + Survivor 压力）。
4. **最小侵入式演进的原则**。

### 4.2 建议修改/调整的部分
1. **目标表述的精准化**：将“仿早期 V8”调整为“**采用 V8 风格的新生代（Scavenge）配合类 HotSpot CMS 风格的非移动老生代（Mark-Sweep）**”。
2. **Phase 0 任务升级**：将“代码审计”升级为“**基于 C++ Private/Getter/Setter 的写入路径重构**”（对应 Q3 决策），作为 Phase 2 的硬性前置屏障。
3. **明确 Side Metadata**：在 Phase 0 直接拍板选择“Side Metadata (Bitmap)”方案，放弃对 `MarkWord` 的进一步压榨。

### 4.3 建议补充的前瞻性考量（可选清单）
1. **大对象空间（Large Object Space, LOS）**：如果遇到几 MB 的 `bytes` 或 `list`，直接在新生代分配会瞬间撑爆 Survivor。建议在架构上预留大对象的判定阈值，超过阈值的对象直接 bypass 新生代，在 OldSpace 分配。
2. **全局句柄（Global Handles）的弱引用管理**：在没有 `__del__` 的前提下，仍需考虑 C++ Embedder 层是否需要弱引用句柄，以及它们在 Sweep 阶段的清理时机。

**结论**：这份计划逻辑严密、风险可控，结合补充的 Q/A 决策，已具备极高的工程指导价值。可以即刻进入 Phase 0 的开发工作。