# Gemini 审查 Prompt：S.A.A.U.S.O OldSpace 架构方案

请你作为一名熟悉 **现代 V8 堆架构**、**分代垃圾回收器**、**页式内存布局**、**remembered set / slot set / card table 设计** 的高级虚拟机工程师，对我为 **S.A.A.U.S.O** 制定的 `OldSpace` 架构方案进行一次**严格、保守、工程化**的技术评审。

你的任务不是直接实现代码，而是判断这份方案是否：

1. 真正适合 S.A.A.U.S.O 当前代码基线。
2. 正确借鉴了现代 V8 的设计思想，而不是误学了不适合当前阶段的复杂度。
3. 在 Phase 1 这个时间点做了合理的取舍，没有过度设计。
4. 能为后续 remembered set、promotion、major `mark-sweep`、future `mark-compact` 提供稳定的物理基础。

请你重点基于以下两个参照系做评审：

- **参照系 A：现代 V8 的 `MemoryChunk + BasePage / MutablePage`、页级 flags、页级 slot set、页级 sweep 元数据设计**
- **参照系 B：S.A.A.U.S.O 当前现实状态**
  - `NewSpace` 仍是连续 semi-space
  - `OldSpace` 尚未实现
  - `WRITE_BARRIER` 尚未启用
  - remembered set 仍未正式启用
  - 当前 Phase 1 目标只是“落地可工作的 OldSpace 基础能力”

请你严格回答以下问题：

1. 这份方案是否正确选择了 **page-based OldSpace** 作为第一阶段的物理布局？
2. 这份方案是否正确判断了：
   - **应该借鉴现代 V8 的 page / chunk 思想**
   - **但不应照搬完整的 `MemoryChunk + BasePage / MutablePage + TypedSlotSet + 并发 sweep` 复杂度**
3. “只让 OldSpace page 化，而 NewSpace 仍保持当前连续 semi-space” 这个取舍是否合理？
4. `remembered set` 第一版只保留“页级挂载点”，而不直接上 `SlotSet / TypedSlotSet / card table`，这个取舍是否正确？
5. 第一版 `OldPageHeader` 的最小字段集是否合理？是否遗漏了关键字段？
6. 第一版是否应直接引入：
   - `free_list`
   - `kLargePage`
   - 页外 metadata 控制块
   - `page-level sweeping` 状态机
7. `OldSpace::Contains()`、`OldPage::FromAddress()`、`Heap::InOldSpace()` 这一组接口是否构成了足够稳定的地址归属闭环？
8. 该方案对 future remembered set / promotion / mark-sweep 的支撑是否足够？还是仍然存在会导致后续返工的结构性问题？

请你输出时采用以下结构：

- **总体结论**
- **建议接受 / 建议修改后接受 / 不建议接受**
- **这份方案最正确的地方**
- **这份方案最危险的地方**
- **与现代 V8 的合理相似点**
- **与现代 V8 的不必要相似点**
- **你认为被遗漏的设计点**
- **你建议修改的条目**
- **如果由你拍板，Phase 1 的最终 OldSpace 路线是什么**

请特别关注以下评审重点：

- `page size` 选 `256 KB` 还是 `512 KB`
- `remembered set` 第一版用 slot list、slot vector，还是应更早使用 bitmap/card table
- 是否应在 Phase 1 就把页式设计做成 `MemoryChunk + metadata` 二分模型
- 是否应在第一版就实现 free list
- 是否需要在第一页方案中引入 `magic / owner / flags` 一类更严格的页校验机制
- 是否应在 Phase 1 预留 `Large Page / LOS` 路线

请不要泛泛地讲“这取决于需求”，而是请你像真正的 VM 架构评审人一样，给出明确偏向和明确判断。

如果你认为该方案基本成立，请明确回答：

**“建议接受该 OldSpace 架构方案，并可作为 S.A.A.U.S.O Phase 1 基线。”**

如果你认为仍存在重大问题，请明确回答：

**“不建议直接接受该方案，需先修改以下关键点。”**

以下是待审查文档：

- [Saauso-OldSpace-Architecture-Proposal.md](file:///e:/MyProject/S.A.A.U.S.O/docs/generational-gc/Saauso-OldSpace-Architecture-Proposal.md)

为了便于你完整审查，请你特别参考该文档中的这些部分：

- 总体结论与设计原则
- Page 大小与地址对齐
- OldPageHeader 最小字段集
- Flags 设计
- 地址归属与快速闭环
- remembered set 设计建议
- 是否采用 `MemoryChunk + metadata_` 二分模型
- 与现代 V8 设计的取舍表
- Phase 1 实施建议
- Gemini 评审建议问题
