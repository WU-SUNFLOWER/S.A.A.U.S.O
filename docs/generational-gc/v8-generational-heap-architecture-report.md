# V8 新生代 & 老生代架构设计调研报告

## 1. 调研目标

本文基于当前仓库源码，对以下问题进行系统整理：

- V8 当前的新生代与老生代分别是如何架构的
- `MemoryChunk` 与 `BasePage` / `MutablePage` 的二分层设计是如何形成的
- `MemoryChunk`、`BasePage`、`MutablePage` 各自有哪些核心字段与核心方法
- 虚拟机在运行时是如何判断一个对象位于新生代还是老生代的

本文聚焦当前源码实现，不讨论历史版本的完整演进细节。

---

## 2. 总体结论

V8 当前这一层的堆架构，可以概括为两条主线：

- **代际维度**：对象堆按年轻代（young generation）与老年代（old generation）组织，不同代采用不同的 GC 策略与空间布局。
- **页元数据维度**：对象并不是直接“挂在某个空间名字下面”，而是先落在某个 `MemoryChunk` 上，再由 `BasePage` / `MutablePage` 元数据对象提供更完整、更可信的页级语义。

运行时判定一个对象是否属于年轻代，并不是靠对象头里保存一个“代际字段”，而是主要通过：

1. 对象地址反推出其所在的 `MemoryChunk`
2. 读取 `MemoryChunk` 上的代际相关 flag
3. 在 sticky mark-bits 模式下，再结合对象 mark bit 与页状态做补充判断

因此，V8 当前更接近一种：

- **对象地址 -> MemoryChunk -> BasePage/MutablePage -> Space / Generation 语义**

的设计，而不是：

- **对象头 -> 直接记录代际**

的设计。

---

## 3. 代际空间总体架构

### 3.1 AllocationSpace 枚举

V8 的空间枚举定义位于 [globals.h](file:///e:/MyProject/v8/src/common/globals.h#L1388-L1414)。当前代码中的核心空间包括：

- `NEW_SPACE`
- `NEW_LO_SPACE`
- `OLD_SPACE`
- `LO_SPACE`
- `CODE_SPACE`
- `CODE_LO_SPACE`
- `SHARED_SPACE`
- `SHARED_LO_SPACE`
- `TRUSTED_SPACE`
- `TRUSTED_LO_SPACE`
- `SHARED_TRUSTED_SPACE`
- `SHARED_TRUSTED_LO_SPACE`
- `RO_SPACE`

从代际视角看，可以粗略整理为：

- **年轻代**
  - `NEW_SPACE`
  - `NEW_LO_SPACE`
- **老年代或非年轻代可写空间**
  - `OLD_SPACE`
  - `LO_SPACE`
  - `CODE_SPACE`
  - `CODE_LO_SPACE`
  - `SHARED_SPACE`
  - `SHARED_LO_SPACE`
  - `TRUSTED_SPACE`
  - `TRUSTED_LO_SPACE`
  - `SHARED_TRUSTED_SPACE`
  - `SHARED_TRUSTED_LO_SPACE`
- **只读空间**
  - `RO_SPACE`

这里需要注意：**“不在年轻代”不总是简单等价于“普通老生代对象”**。例如只读空间、共享空间、trusted space 都有自己的特殊语义。

---

## 4. 新生代架构

### 4.1 NewSpace 是抽象基类

`NewSpace` 定义于 [new-spaces.h](file:///e:/MyProject/v8/src/heap/new-spaces.h#L186-L236)，它是年轻代空间的抽象基类，暴露了：

- 容量接口：`Capacity()`、`TotalCapacity()`、`MinimumCapacity()`、`MaximumCapacity()`
- 增长接口：`Grow(size_t new_capacity)`
- 迭代接口：`begin()` / `end()`
- GC 钩子：`GarbageCollectionPrologue()`、`GarbageCollectionEpilogue()`
- 晋升判定：`IsPromotionCandidate(const MutablePage* page)`

这说明在当前 V8 中，“年轻代”并不是一个硬编码布局，而是一个可替换实现的抽象层。

### 4.2 SemiSpaceNewSpace：保留 from/to 语义，但底层已页化

经典年轻代实现是 `SemiSpaceNewSpace`，其定义与注释位于 [new-spaces.h](file:///e:/MyProject/v8/src/heap/new-spaces.h#L238-L244)。

源码注释仍然保留了“semispace / from-space / to-space”的概念，但实现上，`SemiSpace` 已经不再只是“一个简单连续区间”，而是维护 `memory_chunk_list_` 页链表。对应定义见 [new-spaces.h](file:///e:/MyProject/v8/src/heap/new-spaces.h#L38-L170)。

关键实现特征：

- `SemiSpace::AllocateFreshPage()` 按页向分配器申请新页，见 [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L125-L138)
- `SemiSpace::RewindPages()` 可按页释放回退，见 [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L140-L154)
- `SemiSpace::Swap()` 交换的是页链表与相关状态，而不是单纯交换两个裸地址区间，见 [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L194-L210)
- `SemiSpace::FixPagesFlags()` 会给页面打上 `FROM_PAGE` / `TO_PAGE` 标志，见 [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L156-L174)

因此，当前 V8 的 semispace 更准确地说是：

- **算法语义上仍是 from/to 半空间**
- **物理实现上已经是由一组页构成的 semispace**

### 4.3 PagedNewSpace：新生代的分页化实现

当 `minor_ms` 打开时，V8 使用 `PagedNewSpace`，创建逻辑位于 [heap.cc](file:///e:/MyProject/v8/src/heap/heap.cc#L6246-L6263)。

这表明现代 V8 并不只支持经典的 copying young generation，还支持基于分页空间的新生代实现。也正因为此，年轻代整体的工程基础设施越来越偏向“页”为基本单位。

### 4.4 新生代也有大对象空间

除了 `NEW_SPACE`，年轻代还有 `NEW_LO_SPACE`，实例化逻辑同样在 [heap.cc](file:///e:/MyProject/v8/src/heap/heap.cc#L6260-L6263)。

这意味着：

- 年轻代不只是普通页链表
- 过大的 young object 会进入 young large object space

---

## 5. 老生代架构

### 5.1 Space 是页链表的拥有者

`Space` 定义于 [spaces.h](file:///e:/MyProject/v8/src/heap/spaces.h#L65-L120)。其中最关键的字段是：

- `heap::List<MutablePage> memory_chunk_list_`

这说明普通 paged space 的本体组织方式就是：

- **一个 Space 持有若干 `MutablePage` 组成的链表**

并提供：

- `first_page()`
- `last_page()`
- `memory_chunk_list()`

### 5.2 OldSpace 是典型的 paged old generation space

`OldSpace` 定义于 [paged-spaces.h](file:///e:/MyProject/v8/src/heap/paged-spaces.h#L441-L456)。它代表：

- **老生代普通对象空间**

老生代普通对象主要分布在：

- `OldSpace`
- `CodeSpace`
- `SharedSpace`
- `TrustedSpace`

等不同的 paged space 中。

### 5.3 老生代也有大对象空间

`LargeObjectSpace` 及其派生类定义位于 [large-spaces.h](file:///e:/MyProject/v8/src/heap/large-spaces.h#L29-L218)。

关键子类包括：

- `OldLargeObjectSpace`
- `SharedLargeObjectSpace`
- `TrustedLargeObjectSpace`
- `CodeLargeObjectSpace`

因此，老年代同样不是单一结构，而是：

- 普通页空间
- 大页空间

共同组成。

---

## 6. 为什么 V8 要拆成 MemoryChunk 与 BasePage/MutablePage 两层

### 6.1 MemoryChunk：按地址快速命中的块头

`MemoryChunk` 定义位于 [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L42-L174)。

它的角色是：

- 代表一块按页对齐的内存块
- 在热路径上提供尽可能便宜的页级 flag 查询
- 支持“仅凭对象地址快速反推出所在 chunk”

核心原因是：写屏障、代际判定、marking fast path 这些操作非常热，不适合一上来就进入更复杂的 metadata 对象体系。

### 6.2 BasePage / MutablePage：可信元数据层

`MemoryChunk` 注释里已经明确表达了设计意图：快路径 flag 放在 chunk 上，而更丰富的信息放在对应的 metadata object 上，见 [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L42-L55)。

`BasePage` 和 `MutablePage` 正是这个 metadata 层：

- `BasePage`：所有页的通用元数据基类，见 [base-page.h](file:///e:/MyProject/v8/src/heap/base-page.h#L25-L89)
- `MutablePage`：可写页元数据，加入 remembered set、marking bitmap、页年龄等信息，见 [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L44-L91) 与 [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L286-L371)

### 6.3 二分层的收益

这种分层带来几个好处：

- **快路径便宜**：对象地址直接掩码得到 `MemoryChunk`
- **安全性更高**：复杂可信元数据不必都暴露在 chunk 头上
- **职责更清晰**：
  - `MemoryChunk` 更偏“块头 + 快速 flag”
  - `BasePage` / `MutablePage` 更偏“完整页语义与 GC 管理”
- **更适合 sandbox 设计**：chunk 与元数据对象可以分离，metadata 在 sandbox 下走更可信的访问路径，见 [memory-chunk-inl.h](file:///e:/MyProject/v8/src/heap/memory-chunk-inl.h#L17-L38)

---

## 7. MemoryChunk / BasePage / MutablePage 的内存与对象关系

### 7.1 MemoryChunk 在页块本体上

`MemoryChunk` 的地址反推出方式定义在 [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L146-L165)：

- `BaseAddress(a) = a & ~kAlignmentMask`
- `FromAddress(addr)`
- `FromHeapObject(object)`

这说明 `MemoryChunk` 本身是直接和那块页内存绑定的块头。

对象区的布局起始点由 [memory-chunk-layout.h](file:///e:/MyProject/v8/src/heap/memory-chunk-layout.h#L30-L39) 给出，对普通 data page 来说，对象区是从 `RoundUp(sizeof(MemoryChunk), ...)` 之后开始的。

### 7.2 BasePage / MutablePage 是 side metadata，不是 HeapObject

普通页分配逻辑见 [memory-allocator.cc](file:///e:/MyProject/v8/src/heap/memory-allocator.cc#L438-L455)：

- 先单独 `malloc(sizeof(NormalPage))`
- placement new 构造 `NormalPage`
- 再在 chunk 起始地址上 placement new `MemoryChunk`

这说明：

- `MemoryChunk` 是“页内存本体上的块头”
- `BasePage` / `MutablePage` / `NormalPage` 是“单独分配出来的元数据对象”

两者通过 `Metadata()` / `Chunk()` 互相建立关联：

- `BasePage::FromAddress()` -> `MemoryChunk::Metadata()`，见 [base-page-inl.h](file:///e:/MyProject/v8/src/heap/base-page-inl.h#L15-L23)
- `BasePage::Chunk()` 反向找回 chunk，见 [base-page.h](file:///e:/MyProject/v8/src/heap/base-page.h#L126-L129)

因此，`BasePage` / `MutablePage` 是页的**元数据视图**，但不是页对象区的一部分。

---

## 8. MemoryChunk 的核心字段与核心方法

### 8.1 关键字段

`MemoryChunk` 的核心是页级 flag，定义见 [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L56-L109)。

与代际最相关的标志包括：

- `FROM_PAGE`
- `TO_PAGE`
- `NEW_SPACE_BELOW_AGE_MARK`
- `LARGE_PAGE`
- `IN_WRITABLE_SHARED_SPACE`
- `INCREMENTAL_MARKING`
- `POINTERS_TO_HERE_ARE_INTERESTING`
- `POINTERS_FROM_HERE_ARE_INTERESTING`

这些 flag 支撑了：

- 年轻代判定
- 写屏障快路径
- marking 状态判定
- 是否 large page
- 是否 shared writable page

### 8.2 关键方法

#### 地址映射

- [BaseAddress](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L146-L155)
- [FromAddress](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L158-L160)
- [FromHeapObject](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L162-L165)

#### metadata 获取

- [Metadata(const Isolate*)](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L167-L174)
- [Metadata()](file:///e:/MyProject/v8/src/heap/memory-chunk-inl.h#L49-L70)

#### 代际/页属性判定

- [InYoungGeneration](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L195-L199)
- [IsYoungOrSharedChunk](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L201-L206)
- [InNewSpace](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L245-L249)
- [InNewLargeObjectSpace](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L250-L252)
- [PointersToHereAreInteresting](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L261-L263)
- [PointersFromHereAreInteresting](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L264-L265)

---

## 9. BasePage 的核心字段与核心方法

### 9.1 关键字段

`BasePage` 的核心状态主要在 [base-page.h](file:///e:/MyProject/v8/src/heap/base-page.h#L262-L358)。

比较重要的包括：

- `heap_`：所属 `Heap`
- `owner_`：所属 `BaseSpace`
- `area_start_` / `area_end_`：对象区边界
- `size_`：页大小
- `allocated_bytes_` / `wasted_memory_`
- `high_water_mark_`
- `flags_`

### 9.2 关键方法

#### 地址与对象回溯

- [FromAddress](file:///e:/MyProject/v8/src/heap/base-page-inl.h#L15-L23)
- [FromHeapObject](file:///e:/MyProject/v8/src/heap/base-page-inl.h#L25-L33)

#### 归属与空间

- [heap()](file:///e:/MyProject/v8/src/heap/base-page.h#L77-L80)
- [owner()](file:///e:/MyProject/v8/src/heap/base-page.h#L82-L89)
- [owner_identity()](file:///e:/MyProject/v8/src/heap/base-page-inl.h#L63-L68)

#### 边界与范围

- [Contains](file:///e:/MyProject/v8/src/heap/base-page.h#L92-L100)
- [Chunk()](file:///e:/MyProject/v8/src/heap/base-page.h#L126-L129)
- [Offset(Address)](file:///e:/MyProject/v8/src/heap/base-page.h#L62-L64)

---

## 10. MutablePage 的核心字段与核心方法

`MutablePage` 是可写页的完整 GC 元数据载体。

### 10.1 关键字段

关键字段集中在 [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L286-L338)：

- `slot_set_[]`：remembered set 的普通 slot 集合
- `typed_slot_set_[]`：typed slot 集合
- `marking_progress_tracker_`
- `live_byte_count_`
- `list_node_`：侵入式链表节点
- `possibly_empty_buckets_`
- `allocated_lab_size_`
- `age_in_new_space_`
- `trusted_main_thread_flags_`
- `marking_bitmap_`

其中：

- `slot_set_[]` / `typed_slot_set_[]` 是 remembered set 的页级入口
- `age_in_new_space_` 专门体现 young page 的年龄
- `list_node_` 解释了为什么你在类里没直接看到 `next_page_` / `prev_page_`

### 10.2 关键方法

#### 页 flag 计算与写入

- [OldGenerationPageFlags](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L56-L87)
- [YoungGenerationPageFlags](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L89-L104)
- [SetOldGenerationPageFlags](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L141-L162)
- [SetYoungGenerationPageFlags](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L164-L175)

#### remembered set 管理

- `AllocateSlotSet()` / `ReleaseSlotSet()`，见 [mutable-page.cc](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L210-L228)
- `AllocateTypedSlotSet()` / `ReleaseTypedSlotSet()`，见 [mutable-page.cc](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L230-L247)

#### owner / 页组织

- [owner()](file:///e:/MyProject/v8/src/heap/mutable-page.h#L188-L189)
- `list_node()` 及 `list_node_` 所支持的页链表组织，见 [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L190-L191) 与 [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L310-L310)

---

## 11. 虚拟机如何判断一个对象在新生代还是老生代

这是本文最核心的问题之一。

### 11.1 主入口：HeapLayout::InYoungGeneration

统一入口位于 [heap-layout-inl.h](file:///e:/MyProject/v8/src/heap/heap-layout-inl.h#L19-L59)。

标准路径是：

1. 取对象地址
2. `MemoryChunk::FromHeapObject(object)`
3. 调用 `HeapLayout::InYoungGeneration(chunk, object)`

### 11.2 非 sticky mark-bits 模式

在常规模式下，判定非常直接：

- 看 `chunk->InYoungGeneration()`

而 `MemoryChunk::InYoungGeneration()` 本质就是检查：

- `FROM_PAGE | TO_PAGE`

见 [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L195-L199)。

所以对普通模式来说，对象是否年轻代，主要由其所在页的 chunk flag 决定。

### 11.3 sticky mark-bits 模式

在 sticky mark-bits 模式下，逻辑更复杂，见 [heap-layout.cc](file:///e:/MyProject/v8/src/heap/heap-layout.cc#L16-L23)。

这时会综合：

- page 是否“只包含 old 或 major GC in progress”
- 对象自身的 mark bit 是否已经设置

也就是说，sticky 模式下不能只看页的 `FROM_PAGE` / `TO_PAGE`，还要结合对象状态。

### 11.4 老生代判定

老生代判定并不是简单地“不是 young 就是 old”。  
`Heap::InOldSpace` 位于 [heap-inl.h](file:///e:/MyProject/v8/src/heap/heap-inl.h#L263-L266)：

- 先要求 `old_space_->Contains(object)`
- sticky 模式下还要额外排除 `HeapLayout::InYoungGeneration(object)`

这说明“老生代”在实现上依赖：

- 页所在空间
- 年轻代判定结果

共同决定。

### 11.5 大对象与代际

对象是否在大对象空间，可通过 [HeapLayout::InAnyLargeSpace](file:///e:/MyProject/v8/src/heap/heap-layout-inl.h#L85-L88) 判定，其底层是：

- `MemoryChunk::IsLargePage()`

因此：

- 一个对象既有“年轻/老”语义
- 又有“normal/large page”语义

这两个维度是正交的。

---

## 12. 页是如何被标记成 young / old 的

### 12.1 通用页面初始化

普通页在 [memory-allocator.cc](file:///e:/MyProject/v8/src/heap/memory-allocator.cc#L420-L460) 中创建：

1. 构造 `NormalPage` metadata
2. 构造 `MemoryChunk`
3. 调用 `space->InitializePage(metadata)`

所以具体是 young page 还是 old page，不是 `MemoryAllocator` 单独决定，而是最终由所属 `Space` 的初始化逻辑决定。

### 12.2 Young page

半空间 young page 的标记逻辑见 [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L156-L174)：

- to-space 页会被打上 `TO_PAGE`
- from-space 页会被打上 `FROM_PAGE`

### 12.3 Old page

old generation 页的 flag 计算见 [mutable-page.cc](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L56-L87)。

而 `MutablePage::ComputeInitialFlags()` 会根据 `owner()->identity()` 判断：

- 若是 new space，使用 `YoungGenerationPageFlags`
- 否则使用 `OldGenerationPageFlags`

见 [mutable-page.cc](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L106-L139)。

---

## 13. 对象何时会从 young 晋升到 old

对象当前在 young，不等于它会一直留在 young。

### 13.1 页级与对象级晋升

新生代晋升相关逻辑主要体现在：

- [SemiSpaceNewSpace::ShouldPageBePromoted](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L606-L618)
- [SemiSpaceNewSpace::ShouldBePromoted](file:///e:/MyProject/v8/src/heap/new-spaces-inl.h#L68-L96)

核心依据是：

- 页是否在 age mark 之下
- 地址是否在 age mark 之下

### 13.2 实际页晋升

当一整页从 new 变 old 时，走 [NormalPage::ConvertNewToOld](file:///e:/MyProject/v8/src/heap/normal-page.cc#L57-L70)：

- `ResetAgeInNewSpace()`
- `set_owner(old_space)`
- 清空旧 young flag
- `SetOldGenerationPageFlags`
- 交给 old space 初始化并接管

因此，页的“代际身份”是可以在运行期改变的。

---

## 14. 架构理解图

### 14.1 分层图

```text
HeapObject
  │
  ▼
MemoryChunk
  - 页块头
  - 快速 flag
  - 地址掩码即可命中
  │
  ▼ Metadata()
BasePage
  - heap_
  - owner_
  - area_start_/area_end_
  - size_/allocated_bytes_
  │
  ▼
MutablePage
  - slot_set_/typed_slot_set_
  - marking_bitmap_
  - age_in_new_space_
  - list_node_
  │
  ├─ NormalPage
  └─ LargePage
```

### 14.2 代际判定图

```text
HeapObject
  │
  ▼
MemoryChunk::FromHeapObject(object)
  │
  ▼
读取 chunk flags
  │
  ├─ 非 sticky 模式：FROM_PAGE / TO_PAGE ?
  │
  └─ sticky 模式：页状态 + 对象 mark bit
  │
  ▼
得出：young / 非 young
  │
  ▼
再结合 owner/space 可进一步区分：
old / shared / trusted / read-only / large
```

---

## 15. 结论

### 15.1 关于新生代与老生代

- 当前 V8 的年轻代与老年代都已经是**页化管理**的架构。
- 年轻代保留了 semispace/from-to 抽象，但底层已经是由一组页构成。
- 老年代则更明显地以 paged spaces + large object spaces 组合而成。

### 15.2 关于二分层设计

- `MemoryChunk` 负责**地址可直达的页头语义与快路径 flag**
- `BasePage` / `MutablePage` 负责**完整、可信、较重的页元数据**

这是一种典型的：

- **热路径轻量化**
- **复杂管理外置化**

的系统设计。

### 15.3 关于代际判定

- 对象是否在年轻代，主要不是看对象头，而是看其**所在 `MemoryChunk` 的代际 flag**
- 在 sticky mark-bits 模式下，会进一步结合对象 mark bit
- 因此，V8 是一种**基于页状态驱动对象代际语义**的实现

### 15.4 最值得记住的一句话

在当前 V8 中：

- **对象的代际身份，本质上是它所在页/块的属性投射到对象上的结果。**

---

## 16. 参考源码索引

- [globals.h](file:///e:/MyProject/v8/src/common/globals.h#L1388-L1435)
- [heap.cc](file:///e:/MyProject/v8/src/heap/heap.cc#L6246-L6263)
- [new-spaces.h](file:///e:/MyProject/v8/src/heap/new-spaces.h#L35-L244)
- [new-spaces.cc](file:///e:/MyProject/v8/src/heap/new-spaces.cc#L125-L245)
- [large-spaces.h](file:///e:/MyProject/v8/src/heap/large-spaces.h#L29-L218)
- [spaces.h](file:///e:/MyProject/v8/src/heap/spaces.h#L65-L120)
- [memory-chunk.h](file:///e:/MyProject/v8/src/heap/memory-chunk.h#L42-L265)
- [memory-chunk-inl.h](file:///e:/MyProject/v8/src/heap/memory-chunk-inl.h#L17-L71)
- [base-page.h](file:///e:/MyProject/v8/src/heap/base-page.h#L25-L89)
- [base-page-inl.h](file:///e:/MyProject/v8/src/heap/base-page-inl.h#L15-L33)
- [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L44-L91)
- [mutable-page.h](file:///e:/MyProject/v8/src/heap/mutable-page.h#L286-L371)
- [mutable-page.cc](file:///e:/MyProject/v8/src/heap/mutable-page.cc#L56-L175)
- [memory-allocator.cc](file:///e:/MyProject/v8/src/heap/memory-allocator.cc#L420-L460)
- [heap-layout-inl.h](file:///e:/MyProject/v8/src/heap/heap-layout-inl.h#L19-L59)
- [heap-layout.cc](file:///e:/MyProject/v8/src/heap/heap-layout.cc#L16-L23)
- [heap-inl.h](file:///e:/MyProject/v8/src/heap/heap-inl.h#L242-L266)
- [normal-page.cc](file:///e:/MyProject/v8/src/heap/normal-page.cc#L57-L70)
