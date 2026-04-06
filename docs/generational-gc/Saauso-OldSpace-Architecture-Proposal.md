# S.A.A.U.S.O OldSpace 架构方案

本文档给出 S.A.A.U.S.O 第一阶段 `OldSpace` 的正式架构方案，作为后续人类架构师评审、Gemini 二次评审，以及 Phase 1 实施的统一基线。

## 1. 文档目标

- 为 `OldSpace` 的内存布局、对象分配、页元数据、地址归属判断与后续 remembered set 挂载方式提供正式设计。
- 明确哪些设计借鉴自现代 V8，哪些设计故意不在第一版引入。
- 确保 Phase 1 只解决“可工作的 OldSpace 基础能力”，而不是一次性引入完整的现代分代堆复杂度。

## 2. 背景与问题定义

### 2.1 当前代码基线

- 堆已经预留了连续的 `NewSpace + OldSpace + MetaSpace` 地址布局。
- `NewSpace` 当前仍是简单的双半区 `SemiSpace` 设计。
- `OldSpace` 当前尚未实现真实分配与地址判定。
- remembered set 与 `WRITE_BARRIER` 仍处于关闭状态。

这意味着当前最重要的问题不是“如何做最先进的老生代”，而是“如何设计一个足够稳健、能平滑支撑后续 Phase 2/3 的 OldSpace”。

### 2.2 设计目标

第一阶段 `OldSpace` 必须满足：

- 可分配。
- 可做地址归属判定。
- 可为 future remembered set 提供稳定挂载点。
- 可为 future `mark-sweep` 提供页级统计与空闲块管理基础。
- 不要求并发、增量、typed slot、共享堆、大对象空间等复杂能力。

## 3. 总体结论

本方案建议：

- **采用 page-based 的 OldSpace**
- **采用“Space → Page → Allocatable Area”三级模型**
- **借鉴现代 V8 的 MemoryChunk / Page 思想，但不机械复刻其完整实现**
- **第一版只实现最小页级元数据，不引入并发 sweep、typed slot set、增量标记或共享堆复杂度**

换句话说：

- 要借鉴现代 V8 的**分层方式**
- 不要照搬现代 V8 的**完整机制集合**

## 4. 设计原则

### 4.1 先建立稳定物理布局，再引入复杂 GC 机制

- Phase 1 先解决 “页是什么、对象怎么分配、地址怎么归属、页元数据放哪里”。
- remembered set、promotion、major GC 都应建立在稳定的 OldSpace 物理布局之上。

### 4.2 借鉴现代 V8 的结构，不借鉴其全部复杂度

- 值得借鉴的是：页对齐、地址反推页、页级 flags、页级 owner、页级 remembered-set 挂载点。
- 不值得第一版引入的是：typed slot set、concurrent sweeping、incremental marking、black allocation、shared heap。

### 4.3 OldSpace 优先 page 化，NewSpace 暂不跟进重构

- 第一阶段只要求 OldSpace page 化。
- NewSpace 仍保持当前连续 semi-space 设计。
- 但接口设计上应允许 future 统一到 page / chunk 抽象。

### 4.4 remembered set 的“挂载位置”和“表示形式”解耦

- 第一阶段必须决定 remembered set 挂在哪：挂在页上。
- 第一阶段不必决定 remembered set 的最终最优表示：slot vector、slot list、bitmap、card table 都可后续演进。

## 5. 为什么选择 Page-Based OldSpace

### 5.1 相比“单块连续老生代”的优势

- 地址可快速映射到所属页。
- sweep 可以按页进行。
- free block 可以按页管理。
- remembered set 能自然地成为页级元数据。
- live bytes / fragmentation / allocated bytes 统计更自然。
- future large object、compaction candidate、页级回收都有扩展空间。

### 5.2 相比“完整照搬现代 V8”的优势

- 结构足够稳定。
- 第一版复杂度更可控。
- 更适合当前教学型 / 工程型 VM 的演进节奏。
- 便于在 Phase 1 先完成可工作的 `OldSpace`，而不是陷入全量复杂机制。

## 6. 推荐的分层模型

建议采用如下分层：

- `Heap`
- `OldSpace`
- `OldPage`
- `OldPageHeader`

它们的职责分别是：

- `Heap`
  - 管理整体地址布局
  - 提供 `InOldSpace()` / `InNewSpace()` 一类判定接口
  - 在 future write barrier 中负责跨代入口决策

- `OldSpace`
  - 拥有整个老生代地址范围
  - 管理页链表
  - 决定当前分配页
  - 提供 `Setup / TearDown / AllocateRaw / Contains`

- `OldPage`
  - 表示一个固定大小的老生代页
  - 提供页内分配区、页级统计、页级 remembered-set、页级 free list 入口

- `OldPageHeader`
  - 页头最小元数据
  - 位于页首固定区域
  - 支撑地址反推、归属判断、统计、调试断言

## 7. Page 大小与地址对齐

### 7.1 推荐 Page 大小

第一阶段建议：

- `kOldPageSize = 256 KB`
- 必须是 `2` 的幂

选择理由：

- 更容易在小规模测试中尽早触发换页、边界条件与 future GC 路径。
- 对教学型 / 工程型 VM 更友好，便于尽早暴露分配与归属判断问题。
- 当前 remembered set 与 free list 仍未正式启用，`512 KB` 的元数据摊销收益暂时不显著。

### 7.2 对齐要求

- 每个页必须按 `kOldPageSize` 对齐。
- 这样可支持通过地址快速反推出页基址：

```cpp
page_base = addr & ~(kOldPageSize - 1)
```

这是本方案最关键的基础能力之一。

## 8. OldPageHeader 最小字段集

第一版建议 `OldPageHeader` 至少包含以下字段：

- `uint32_t magic`
- `uintptr_t flags`
- `OldSpace* owner`
- `OldPage* next`
- `OldPage* prev`
- `Address area_start`
- `Address area_end`
- `Address allocation_top`
- `Address allocation_limit`
- `size_t allocated_bytes`
- `size_t live_bytes`
- `void* remembered_set`
- `void* free_list_head`

### 8.1 字段解释

- `flags`
  - 页状态标志位
  - 第一版只保留少量必要枚举

- `magic`
  - 页合法性魔数
  - 用于 `FromAddress()` 后的防御性校验
  - 防止野指针或越界地址被误识别为合法页

- `owner`
  - 所属 `OldSpace`
  - 便于调试、断言、future sweep 与 page iteration

- `next / prev`
  - 形成双向页链表
  - 便于遍历、统计、回收与 future page reordering

- `area_start / area_end`
  - 可分配区域边界
  - 与页头分离，避免把 header 本身计入 allocatable area

- `allocation_top / allocation_limit`
  - 第一版先支持页内线性分配

- `allocated_bytes`
  - 统计页内已分配总字节数

- `live_bytes`
  - 先保留字段
  - 供 future `mark-sweep` 使用

- `remembered_set`
  - 第一版作为挂载点保留
  - 后续可接 slot list、slot vector、bitmap 或 card table

- `free_list_head`
  - 第一版可仅作占位 / 简单空闲块入口
  - 供 future sweep 产出空闲块后复用

## 9. Flags 设计

### 9.1 第一版应保留的最小标志

建议第一版只引入这些标志：

- `kNoFlags`
- `kOldPage`
- `kLargePage`
- `kPointersFromHereAreInteresting`
- `kPointersToHereAreInteresting`
- `kNeedsSweep`
- `kSwept`

### 9.2 暂不引入的 V8 风格标志

第一版不建议引入：

- `INCREMENTAL_MARKING`
- `BLACK_ALLOCATED`
- `EVACUATION_CANDIDATE`
- `IN_WRITABLE_SHARED_SPACE`
- 与 young page 完整联动的 `FROM_PAGE / TO_PAGE`

原因：

- 这些标志都服务于更复杂的并发、增量、共享堆或年轻代 page 化设计。
- 当前 S.A.A.U.S.O 还不具备引入这些概念的收益条件。

## 10. 地址归属与快速闭环

### 10.1 必须提供的接口

建议 Phase 1 提供：

- `OldPage* OldPage::FromAddress(Address addr)`
- `bool OldSpace::Contains(Address addr)`
- `bool Heap::InOldSpace(Address addr)`

### 10.2 设计目标

对任意 old-space 对象地址，应可以：

1. 先判定该地址是否位于老生代整体地址范围。
2. 再通过页对齐快速得到页基址。
3. 再校验页头 `magic`。
4. 再校验页头 `owner` 指向当前 `OldSpace`。
5. 最后再通过页头获取页元数据。

这样 future write barrier 就能实现如下逻辑：

- 通过宿主对象地址定位宿主页
- 通过宿主页元数据定位该页 remembered set
- 将 old-to-new slot 记录到该页的 remembered set 中

### 10.3 防御性要求

`OldSpace::Contains()` 不应只做“落在 old generation 总体地址范围内”的粗判定。  
在需要严格确认某地址是否属于有效 old-space 对象时，应结合：

- `OldPage::FromAddress()`
- `magic` 校验
- `owner == this` 校验

这样可以有效防止栈地址、C++ 堆地址、损坏对象地址被误判为 OldSpace 地址。

### 10.4 与当前 NewSpace 的关系

Phase 1 不要求 NewSpace 也做 page 化。

当前推荐做法是：

- old-host page 通过页对齐定位
- target 是否在 new space 继续用区间判断

这已经足够支撑第一版 remembered set 设计。

## 11. 分配策略

### 11.1 第一版推荐策略

第一版 `OldSpace::AllocateRaw()` 建议采用：

- **优先页内线性分配**
- **后续再接 free list**

也就是说：

1. 优先在当前活动页的 `allocation_top` 到 `allocation_limit` 之间 bump allocate。
2. 当前页空间不足时，切换到下一页或新建可用页。
3. 当前所有页都无法满足时，返回 `kNullAddress`，由上层决定触发 future major GC 或 OOM。

此外，第一版必须增加常规对象大小上限保护：

- `size_in_bytes` 不能超过 `kMaxRegularHeapObjectSize`
- `kMaxRegularHeapObjectSize` 应明显小于 `kOldPageSize - sizeof(OldPageHeader)`

若超出阈值：

- 第一版直接触发断言、OOM 或 Fatal Error
- 不允许静默溢出到页外

### 11.2 为什么第一版不强制 free list

因为 Phase 1 的目标是“可工作的老生代基础能力”，而不是“可回收并复用一切碎片”。

因此：

- 可以先保留 `free_list_head` 字段与抽象
- 但不强制第一版完整实现 free list 分配器

## 12. remembered set 设计建议

### 12.1 应该借鉴什么

现代 V8 最值得借鉴的点不是具体 `SlotSet` 类型名，而是：

- remembered set 的元数据应该挂在页上

### 12.2 第一版不建议做什么

第一版不建议直接实现：

- 多 remembered-set 类型数组
- `TypedSlotSet`
- card table
- 页头内联复杂 bitmap 扫描逻辑

### 12.3 第一版建议做什么

第一版建议：

- `OldPageHeader` 中保留 `remembered_set` 挂载点
- remembered set 的具体实现第一版明确采用：
- `Slot Vector`
- 即简单的指针数组 / 向量，等价于 `std::vector<Address*>` 风格结构

理由：

- 更容易验证正确性
- 更容易打印调试信息
- 更适合当前 Phase 2 的 barrier 启用阶段
- 更符合 Gemini 评审给出的“先透明、后压缩”的结论

### 12.4 未来演进

后续如果 remembered set 正确性跑稳，再升级为：

- slot bitmap
- card table
- 压缩型页级记录结构

## 13. 是否采用 `MemoryChunk + metadata_` 二分模型

### 13.1 我的建议

**借鉴思想，但不要求第一版完整照搬。**

### 13.2 理由

现代 V8 的好处是：

- 内存页实体与控制块分离
- 元数据可以在 C++ 堆中灵活组织
- 更适合复杂并发 / 多种 page 类型

但 S.A.A.U.S.O 当前第一版并不需要这么重的体系。

### 13.3 第一版推荐实现方式

推荐两种可接受方案：

- **方案 A：最小内联页头**
  - 页头直接位于页首
  - 简单直接
  - 调试方便

- **方案 B：页首最小头 + C++ 控制对象**
  - 兼容 future 扩展
  - 比完整 `MemoryChunk / BasePage / MutablePage` 更轻

本方案建议优先：

- **方案 A**

因为它最符合第一版“先把物理布局做稳”的目标。

## 14. 与现代 V8 设计的取舍表

### 14.1 建议现在就借鉴

- page-based old space
- 页对齐与地址反推页
- 页级 flags
- 页级 owner / area / allocated_bytes
- 页级 remembered-set 挂载点
- 页链表

### 14.2 建议现在只预留，不实现

- `kLargePage`
- 页状态枚举
- future young/old 统一 chunk 抽象
- future free list / sweep 协同扩展位
- 超大对象分配的显式拦截策略

### 14.3 建议暂不借鉴

- `TypedSlotSet`
- 并发 sweeping 状态机
- 增量标记相关 flags
- black allocation
- evacuation candidate
- fully paged young generation
- shared heap 相关 flags

## 15. Phase 1 实施建议

### 15.1 必须完成

- `OldSpace::Setup`
- `OldSpace::TearDown`
- `OldSpace::AllocateRaw`
- `OldSpace::Contains`
- `Heap::InOldSpace()` 或等价入口
- `OldPage::FromAddress()`
- 页链表与最小页头

### 15.2 建议同步完成

- 最小 flags 集合
- `allocated_bytes` 统计
- `remembered_set` 挂载点
- `free_list_head` 预留字段
- `magic` 合法性校验
- `kMaxRegularHeapObjectSize` 断言或 OOM 拦截

### 15.3 可以延后

- remembered set 真正启用
- free list 真正参与分配
- old-gen `mark-sweep`
- page-level sweeping 状态机

## 16. Gemini 评审建议问题

建议重点评审以下问题：

1. 第一版 page size 选 `256 KB` 还是 `512 KB` 更合适？
2. 第一版是否需要真正实现 free list，还是仅保留预留字段即可？
3. remembered set 第一版应选 slot vector 还是 slot list？
4. 是否有必要在第一版引入页外 C++ 控制块，而不是纯内联页头？即是否采用现代 V8 的`MemoryChunk+BasePage/MutablePage`二分模型？
5. `OldSpace::Contains()` 是否只依赖整体范围判断，还是要同时校验页头魔数 / owner 一致性？
6. 是否在 Phase 1 就预留 `kLargePage` 路线？

## 17. 最终建议

S.A.A.U.S.O 第一阶段的 `OldSpace` 最合适的路线是：

- **采用 page-based 的 OldSpace**
- **以页为 remembered set、统计和 future sweep 的基本单元**
- **借鉴现代 V8 的 chunk/page 思想，但不照搬其完整复杂度**
- **第一版采用 256 KB 页、内联页头、slot vector remembered set 与 magic 防御校验**
- **先做稳定的页式物理布局，再做 barrier、promotion 和 major GC**

这条路线既能保证未来演进空间，也不会让 Phase 1 被过度设计拖垮。
