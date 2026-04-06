# S.A.A.U.S.O YoungSpace Page 化改造方案

本文档给出 S.A.A.U.S.O 下一阶段关于 `YoungSpace` 页式化改造的正式技术方案，作为后续人类架构师评审、Gemini 严格审查，以及实际落地开发的统一基线。

## 1. 文档目标

- 解释为什么当前 `NewSpace` 的“连续裸区间”模型已经开始阻碍 future 写屏障与分代 GC 演进。
- 明确为什么需要引入 `BasePage / NewPage / OldPage` 的统一页元信息体系。
- 给出一份分步可落地的改造方案，确保当前系统可以平滑从“young semi-space”演进到“fixed-size paged young space”。
- 明确哪些内容本阶段必须完成，哪些只做预留，不引入过度复杂度。

## 2. 背景与问题定义

### 2.1 当前代码基线

- `NewSpace` 目前仍然是两块连续 `SemiSpace`，由 `Heap::Setup()` 在一大段连续虚拟地址中切出，见 [heap.cc:L37-L56](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc#L37-L56) 与 [spaces.h:L44-L68](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.h#L44-L68)。
- `OldSpace` 已经具备页式布局、页头元数据、free list 与 remembered set 挂载点，见 [spaces.h:L72-L189](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.h#L72-L189) 与 [spaces.cc:L74-L334](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.cc#L74-L334)。
- `MarkSweepCollector` 已经从 `OldSpace` 中解耦出来，专门承载 old-gen 的扫描与 sweep 逻辑，见 [mark-sweep-collector.h](file:///e:/MyProject/S.A.A.U.S.O/src/heap/mark-sweep-collector.h) 与 [mark-sweep-collector.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/mark-sweep-collector.cc)。
- `WRITE_BARRIER` 仍处于关闭状态，见 [heap.h:L109-L118](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.h#L109-L118)。

### 2.2 当前结构性问题

当前 S.A.A.U.S.O 中，若要在写屏障中判断：

- 宿主对象是否在老生代
- 被写入值是否在新生代

两者并不对称：

- `OldSpace` 可以通过地址直接反推出 `OldPage`，再读页头元信息完成判断。
- `NewSpace` 仍然依赖 `Heap::InNewSpaceEden()` / `Heap::InNewSpaceSurvivor()` 这类区间判断接口，背后依赖 `Heap` 持有的整体区间信息。

这会带来两个现实问题：

- 写屏障如果依赖 `Isolate::Current()` 再拿到 `Heap`，则会把 TLS 访问引入高频路径。
- 若要求所有写屏障调用点显式传入 `Isolate*` 或 `Heap*`，则会污染大量 setter / 写字段路径，既不优雅，也不现实。

### 2.3 真实结论

问题的根因不是“写屏障 API 还不够好”，而是：

- **当前新生代对象缺乏和老生代对称的 page metadata 模型**

因此本次改造的根本目标不是“把 NewSpace 改得更现代”，而是：

- **让 young / old / meta 都能通过地址 → page metadata 完成 O(1) 空间与代际判定**

## 3. 总体结论

本方案建议：

- **抽出 `BasePage` 作为页公共基建**
- **引入 `NewPage` 与 `OldPage`，统一纳入 page metadata 体系**
- **让 `BasePage::flags_` 成为 future 写屏障的 O(1) 代际判定基础**
- **将 `NewSpace` 从“两块裸连续区间”改造成“固定大小、固定页数的 young page 集合”**
- **第一版不实现 young 扩容，不实现复杂 page 调度，不实现现代 V8 式的 fully dynamic young generation**
- **后续再将 scavenge 逻辑从 `Heap` 中解耦为 `ScavengerCollector`**

换句话说：

- 要解决的是 **判定机制和元信息模型**
- 不是一上来追求 **完整现代 V8 young generation 复杂度**

## 4. 为什么现在必须做

### 4.1 写屏障不能长期依赖 `Isolate::Current()`

写屏障是高频路径。  
在高频路径中引入：

- TLS 访问
- 当前线程 isolate 查找
- 再经由 isolate 找 heap
- 再做 young 空间区间判断

这在工程上不可接受。

### 4.2 代际判定必须尽快从“全局上下文依赖”转向“对象地址自描述”

future barrier 的理想形态应当是：

1. 由宿主对象地址找到所在页。
2. 由被写入值地址找到所在页。
3. 读取两页的 `flags_`。
4. O(1) 判断是否属于 `old -> young`。

这要求 young page 和 old page 拥有统一的 metadata 入口。

### 4.3 不做这一步，后续多个方向都会被拖累

如果不先解决 young page metadata 问题，下面这些后续工作都会背负结构性包袱：

- `WRITE_BARRIER`
- remembered set 正式启用
- promotion
- old-to-new 边记录
- future `ScavengerCollector`
- future major / minor collector 的统一 page/chunk 判定体系

## 5. 为什么这个改造是可行的

### 5.1 OldSpace 已经证明了 page metadata 路线可行

当前 `OldSpace` 已经落地了：

- 页头元数据
- `magic / flags / owner`
- `next / prev`
- `area_start / area_end`
- `allocation_top / allocation_limit`
- free list
- page-local remembered set

见 [spaces.h:L103-L172](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.h#L103-L172)。

这说明 page 化不是纸面方案，而是仓库里已经被验证可行的结构。

### 5.2 `MarkSweepCollector` 解耦已经证明 GC 逻辑可从 Space 中分离

当前 old-gen 路线已经完成：

- `OldSpace` 负责内存管理原语
- `MarkSweepCollector` 负责扫描与回收调度

这为下一步：

- `NewSpace` 负责 young memory layout
- `ScavengerCollector` 负责复制与转发

提供了直接可复用的架构模式。

### 5.3 第一版 YoungSpace 不扩容，大幅降低爆炸半径

本方案明确约束：

- VM 启动后 young 总大小固定
- young 总页数固定
- 暂不实现 grow / shrink

这意味着本次改造只关注：

- page metadata
- semispace page 组织
- O(1) 判定入口

而不引入动态容量管理复杂度。

## 6. 总体设计原则

### 6.1 先统一页元信息，再统一 collector

改造顺序必须是：

1. 先抽 `BasePage`
2. 再引入 `NewPage`
3. 再让 `NewSpace` page 化
4. 最后再把 `Scavenge` 从 `Heap` 中剥离

而不是反过来。

### 6.2 先解决“判定闭环”，再解决“调度复杂度”

本次优先级最高的是：

- `BasePage::FromAddress()`
- `flags_` 驱动的 O(1) 判定

而不是：

- young 扩容
- 并发 scavenger
- 更复杂的 page 生命周期

### 6.3 保持 current semispace 语义不变

第一版虽引入 `NewPage`，但 `NewSpace` 仍然保持：

- 两个半区
- 一边 from，一边 to
- scavenge 后 flip

只是这两个半区不再是“裸地址区间”，而是“固定页集合”。

## 7. 目标分层

建议最终分层如下：

- `Heap`
- `BasePage`
- `NewPage`
- `OldPage`
- `NewSpace`
- `OldSpace`
- `MarkSweepCollector`
- `ScavengerCollector`（后续阶段）

职责分别是：

- `Heap`
  - 持有 spaces 与 collectors
  - 负责整体地址布局
  - 提供 future barrier 的统一入口

- `BasePage`
  - 承载 page 公共元数据
  - 提供统一的地址反推入口

- `NewPage`
  - 承载 young page 的语义
  - 通过 flags 标识自己处于 from / to

- `OldPage`
  - 承载 old page 的 free list / remembered set 等老生代专属能力

- `NewSpace`
  - 管理固定数量的 young pages
  - 维护两个 semispace 页集合

- `OldSpace`
  - 管理老生代页链表与内存原语

- `MarkSweepCollector`
  - 负责 old-gen 扫描与回收

- `ScavengerCollector`
  - 后续承载 young-gen 复制、转发与 flip 逻辑

## 8. `BasePage` 设计

### 8.1 放置位置

- `src/heap/base-page.h`
- `src/heap/base-page.cc`

### 8.2 第一版建议字段

- `uint32_t magic_`
- `uintptr_t flags_`
- `Space* owner_`
- `BasePage* next_`
- `BasePage* prev_`
- `Address area_start_`
- `Address area_end_`
- `Address allocation_top_`
- `Address allocation_limit_`
- `size_t allocated_bytes_`

其中：

- `allocated_bytes_` 保留在 `BasePage` 中，用于表达页内已经消耗的分配水位统计。
- `live_bytes_` **不建议放入第一版 `BasePage`**。它更接近 old-gen mark-sweep 的统计结果，而不是 young/old 共通的页元属性。
- 如果后续 old-gen 的 `MarkSweepCollector` 需要 `live_bytes`，建议优先放到：
  - `OldPage` 自身的扩展字段
  - 或 collector 的 sweep 统计结构
  - 而不是一开始塞进 `BasePage`

### 8.3 为什么 `owner_` 应升级为 `Space*`

当前 `OldPage` 的 `owner_` 是 `OldSpace*`。  
一旦引入 `NewPage`，若仍各自保留不同类型 owner，就会破坏公共抽象。

因此建议：

- `BasePage::owner_` 统一使用 `Space*`

这样：

- `NewPage` 可归属于 `NewSpace`
- `OldPage` 可归属于 `OldSpace`

同时仍保留调试断言和归属校验能力。

### 8.4 统一入口

建议 `BasePage` 提供：

- `static Address PageBase(Address addr)`
- `static BasePage* FromAddress(Address addr)`

这是整个改造最关键的 API 之一。

## 9. `flags_` 方案

### 9.1 第一版建议最小集合

- `kNoFlags`
- `kEdenPage`
- `kSurvivorPage`
- `kOldPage`
- `kMetaPage`

### 9.2 这些 flag 的作用

- `kEdenPage / kSurvivorPage / kOldPage / kMetaPage`
  - 解决空间归属判定（新生代空间通过`kEdenPage | kSurvivorPage`进行判定）

- `kEdenPage / kSurvivorPage`
  - 解决 minor GC 中 semispace 角色判定

### 9.3 第一版不建议引入

- `kPointersFromHereAreInteresting`
- `kPointersToHereAreInteresting`
- `kNeedsSweep`
- `kSwept`
- `INCREMENTAL_MARKING`
- `BLACK_ALLOCATED`
- `EVACUATION_CANDIDATE`
- `IN_WRITABLE_SHARED_SPACE`

原因：

- `kPointersFromHereAreInteresting / kPointersToHereAreInteresting` 更偏向 future barrier 热路径优化，不是第一版建立 page metadata 闭环的必要条件。
- `kNeedsSweep / kSwept` 更偏向 old-gen sweep 生命周期状态，不属于 young/old 共通的最小页标记集合。
- 其余标志与 old-space 路线一致，当前收益不够高。

## 10. `NewPage` 设计

### 10.1 目标

`NewPage` 的目标不是在第一版承载复杂 young 元数据，而是：

- 成为 young page 的显式类型
- 拥有和 `OldPage` 对称的公共 page 元属性
- 通过 flags 表达 from / to 角色

### 10.2 第一版建议

第一版 `NewPage` 可以非常薄：

- 继承 `BasePage`
- 不额外引入复杂字段
- 只依赖 `flags_` 区分：
  - `kFromPage`
  - `kToPage`

### 10.3 为什么这是合理的

因为第一版的 young page 化目标是：

- 解决写屏障与空间判定问题
- 不是立即引入复杂 young allocator 或 dynamic page lifecycle

## 11. `OldPage` 改造建议

### 11.1 保留内容

`OldPage` 继续保留：

- remembered set
- free list
- old-gen 专属操作

### 11.2 需要做的改造

- 继承 `BasePage`
- 把公共字段迁出
- 自身只保留 old-gen 专属元数据与方法

### 11.3 目标结果

重构后：

- `BasePage` 表达“页是什么”
- `OldPage` 表达“老生代页多出来的专属能力是什么”

## 12. `NewSpace` 改造方案

### 12.1 保持 fixed-size

第一版明确：

- `NewSpace` 总大小固定
- 页总数固定
- VM 启动后不扩容

### 12.2 从“裸半区”改成“半区页集合”

当前：

- `eden_space_`
- `survivor_space_`

改造后建议变成：

- `from_pages_`
- `to_pages_`

或者保留原命名但底层改成 page 集合，两者皆可。  
从 future scavenger 语义一致性看，我更建议：

- **直接改成 `from/to` 术语**

### 12.3 第一版管理方式

建议：

- young 总地址范围在 `Heap::Setup()` 时仍一次性切出
- 再把这块范围按固定 page size 切成若干 `NewPage`
- 前半作为 `from_pages`
- 后半作为 `to_pages`

### 12.4 为什么这样可行

这样做既保留了：

- 固定 young 总大小
- semispace flip 语义

又获得了：

- young page metadata
- 通过地址反推 page 的能力

## 13. page size 选择

### 13.1 建议

第一版建议：

- `BasePage`、`NewPage`、`OldPage` 统一使用 `256 KB`

### 13.2 原因

- 与当前 `OldPage` 保持一致，便于统一 `BasePage::FromAddress()`
- 统一页对齐掩码，减少 future barrier 与 collector 复杂度
- 在默认 young size 下，页数适中，测试中更容易触发跨页边界

## 14. 对写屏障的直接收益

### 14.1 改造前

future barrier 若要判断 value 是否在 young，仍需要依赖：

- `Heap`
- `NewSpace` 区间
- 甚至可能引入 `Isolate::Current()`

### 14.2 改造后

future barrier 的理想入口可变成：

1. `BasePage::FromAddress(host.ptr())`
2. `BasePage::FromAddress(value.ptr())`
3. 读 `flags_`
4. 判断是否：
   - host 在 old
   - value 在 young

这正是本次改造最核心的收益。

## 15. 与 `ScavengerCollector` 的关系

### 15.1 为什么不在本次一起做

因为本次最根本的问题是：

- young 缺乏 page metadata

而不是：

- scavenge 算法实现得不够独立

### 15.2 但必须预留

本次改造完成后，下一步就应当是：

- 把 `Heap::DoScavenge()` 里的复制 / 转发 / flip 逻辑逐步迁移到 `ScavengerCollector`

### 15.3 最终目标

最终形成与 old-gen 对称的结构：

- `MarkSweepCollector`
- `ScavengerCollector`

## 16. 推荐分步落地路径

### 16.1 第一步：抽 `BasePage`

目标：

- 从当前 `OldPage` 中抽出公共 page 元字段
- 不改变 old-gen 行为

### 16.2 第二步：让 `OldPage` 继承 `BasePage`

目标：

- 保持当前测试全部不变
- 验证 page 公共抽象不会破坏现有 old-gen 功能

### 16.3 第三步：引入 `NewPage`

目标：

- 定义 young page 类型
- 建立 `kYoungPage / kFromPage / kToPage`

### 16.4 第四步：把 `NewSpace` 改成固定 young page 集合

目标：

- 不改 scavenger 算法语义
- 只改底层物理布局与空间判定模型

### 16.5 第五步：统一地址 → page 判定入口

目标：

- 让 future barrier 不再依赖 `Isolate::Current()`

### 16.6 第六步：再解耦 `ScavengerCollector`

目标：

- 把 current `Heap::DoScavenge()` 从 Heap 主体中迁出

## 17. 本阶段必须完成

- `BasePage`
- `NewPage`
- `OldPage` 继承 `BasePage`
- `flags_` 的 young/old/meta/from/to 基础判定能力
- fixed-size page-based `NewSpace`
- 统一 `FromAddress()` 入口

## 18. 本阶段明确不做

- young 扩容 / 缩容
- fully dynamic young page 数量调整
- concurrent scavenging
- minor-ms
- shared heap
- card table 正式启用
- `WRITE_BARRIER` 正式打开
- `ScavengerCollector` 全量落地

## 19. 风险与控制

### 19.1 最大风险

- 一边做 young page 化，一边意外把 scavenger 算法也全量改写

### 19.2 控制方式

- 严格把本次改造限制为：
  - page metadata
  - page-based `NewSpace`
  - O(1) 判定闭环

### 19.3 第二个风险

- `BasePage` 抽象过早塞入太多 old-specific / young-specific 字段

### 19.4 控制方式

- `BasePage` 只放公共元数据
- `OldPage` / `NewPage` 各自保留专属语义

## 20. 人类与 Gemini 联合评审重点

建议重点评审以下问题：

1. `BasePage::owner_` 是否应统一为 `Space*`？
  - 人类架构师意见：可以引入一个过度类`class PagedSpace : public Space { ... }`，然后将`BasePage::owner_`统一为 `PagedSpace*`
  - Gemini意见：（TODO）
2. `BasePage` 是否应承担 `magic_` 与 `allocated_bytes_ / live_bytes_`？
  - 人类架构师意见：`magic_` 与 `allocated_bytes_` 保留在 `BasePage`；`live_bytes_` 不进入第一版 `BasePage`
  - Gemini意见：（TODO）
3. young page 与 old page 是否应统一 `256 KB` 页大小？
  - Gemini意见：（TODO）
4. `NewSpace` 第一版是否应直接从 `eden/survivor` 术语切到 `from/to`？
  - 人类架构师意见：建议继续沿用 `eden/survivor` 术语。个人感觉比 V8 中的 `from/to` 更加清晰直白，可读性更好。
  - Gemini意见：（TODO）
5. 是否需要在第一版就把 `MetaSpace` 纳入 page metadata 闭环？
  - Gemini意见：（TODO）
6. `ScavengerCollector` 是否应在 YoungSpace page 化之后立刻开始拆？
  - 人类架构师意见：建议放在 YoungSpace page 化和 MetaSpace page 化之后，单独开一个 Issue/MergeRequest 来做
  - Gemini意见：（TODO）

## 21. 最终建议

S.A.A.U.S.O 当前最正确的下一步，不是继续在现有裸 `SemiSpace` 之上打补丁，而是：

- **尽快建立统一的 `BasePage` 元信息体系**
- **让 `NewSpace` 与 `OldSpace` 共享地址 → page metadata 的 O(1) 闭环**
- **在保持 fixed-size young generation 的前提下，完成 young page 化**
- **为 future 写屏障与 `ScavengerCollector` 解耦扫清结构障碍**

这项改造不是“追求现代 V8 外观”，而是在解决一个已经真实影响 future barrier 落地的架构性问题。
