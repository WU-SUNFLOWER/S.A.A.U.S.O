# S.A.A.U.S.O Phase 0 地址语义审计

本文档用于固化 Phase 0 对“对象物理地址是否参与语言语义”的审计结果，为后续 promotion、old generation 和 future moving GC 提供边界约束。

## 1. 总体结论

- 当前代码中仍存在少量直接依赖对象地址的实现。
- 这些实现可以分为两类：
- 调试可见性：主要是默认 `repr`
- 语言语义：主要是部分 `hash`
- 当前仓库尚未实现 `id()` builtin，这是 Phase 0 最宝贵的冻结窗口。
- Phase 0 的策略不是立刻全面改写这些实现，而是先冻结约束：
- 不再新增新的“基于物理地址”的语言语义点
- future `id()` 不依赖对象物理地址
- future moving GC 不应被当前地址语义长期绑死

## 2. 已识别的地址绑定点

### 2.1 默认对象 `repr`

- 文件：
- [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)

- 当前行为：
- 默认对象 `repr` 会输出对象地址

- 分类：
- 调试可见性

- 风险：
- 如果 future mark-compact 发生对象移动，输出地址将变化
- 这不一定破坏语言语义，但会影响调试输出稳定性

### 2.2 function / method `repr`

- 文件：
- [runtime-py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-function.cc)

- 当前行为：
- function / method 的默认 `repr` 也会输出地址

- 分类：
- 调试可见性

- 风险：
- 与默认对象 `repr` 相同

### 2.3 `type.__hash__`

- 文件：
- [py-type-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc)

- 当前行为：
- 直接使用对象地址作为 hash

- 分类：
- 语言语义

- 风险：
- 这是 future moving GC 的直接阻碍点之一

### 2.4 `None.__hash__`

- 文件：
- [py-oddballs-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-oddballs-klass.cc)

- 当前行为：
- 直接使用对象地址作为 hash

- 分类：
- 语言语义

- 风险：
- 与 `type.__hash__` 相同

## 3. 当前未实现但必须冻结方向的点

### 3.1 `id()`

- 当前状态：
- 仓库尚未实现 `id()` builtin

- Phase 0 决议：
- future `id()` 不依赖对象物理地址
- 若后续实现，应基于独立 identifier、side metadata 或 side table

### 3.2 `__del__`

- 当前状态：
- 当前对象模型没有 `__del__` / finalizer 槽位

- Phase 0 决议：
- 不把 `__del__` 纳入当前 VM 主路线
- 资源管理优先采用显式协议与 `with`

## 4. 冻结约束

- 不新增任何新的地址型 `hash`
- 不实现“返回物理地址”的 `id()`
- 不把 `repr` 的地址输出当作 future major GC 的兼容负担
- 对已有地址型 `hash`，在进入 future moving GC 前必须先迁移

## 5. 对 Phase 1 的影响

- `OldSpace + non-moving mark-sweep` 不会立刻被这些地址语义阻塞
- 但它们会直接影响 future promotion 之后的 moving major GC 讨论
- 因此 Phase 1 可以继续推进，但地址语义问题不能在 Phase 0 后失去跟踪

## 6. 一句话结论

当前地址语义问题还没有阻止 Phase 1 的 `OldSpace` 落地，但已经足够构成 future moving GC 的明确前置约束。  
Phase 0 的正确做法不是假装这些问题不存在，而是冻结边界、禁止继续扩大，并把 future `id/hash` 演进方向明确写入文档。
