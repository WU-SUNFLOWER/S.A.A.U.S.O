# PyList 尾部对象保活修复与回归测试计划

## Summary

本计划聚焦修复 `PyList` 在逻辑删除元素后仍通过尾槽持有对象引用、导致对象被额外保活的问题，并补充单元测试防止后续回退。

目标：

* 修复 `PyList::PopTagged()`、`PyList::RemoveByIndex()`、`PyList::Clear()` 造成的尾部对象残留引用问题。

* 保持 `PyList` 现有公开行为与主语义不变，只清理“逻辑已删除元素”的底层引用。

* 添加对象层与 GC 层回归测试，验证修复后不会再出现“删除后对象仍被保活”的情况。

成功标准：

* `PyList` 删除/清空后，对应尾槽被显式置空。

* 相关既有单测继续通过。

* 新增回归测试可稳定证明：

  * `Pop/RemoveByIndex/Clear` 后长度语义正确；

  * 删除掉的对象不再因为 `PyList` 尾槽残留而被保活；

  * minor GC 后容器行为不回退。

## Current State Analysis

### 1. 当前实现中的问题点

`PyList` 当前存在 3 处逻辑删除后未清尾槽的问题：

* `PyList::PopTagged()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L39-L42)

  * 当前逻辑仅 `--length_` 后返回旧尾元素，未把原尾槽置空。

* `PyList::RemoveByIndex()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L84-L92)

  * 当前逻辑会把后续元素前移，但最后一个已失效槽位未清空。

* `PyList::Clear()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L105-L107)

  * 当前逻辑仅把 `length_` 置 0，整个底层 `FixedArray` 仍保留旧对象引用。

这些实现不会直接破坏当前 GC 正确性，但会让“已从 Python 语义上删除的对象”继续被 `PyList` 的底层 `FixedArray` 引用，从而被额外保活。

### 2. 当前相关实现与约束

* `PyList` 底层存储是 `FixedArray`，接口定义见 [py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h#L19-L98)。

* `FixedArray::Set()` 支持写入 `Tagged<PyObject>`，且当前堆模型下这是清尾槽的标准方式，见 [fixed-array.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.cc#L38-L46)。

* `PyList` 当前已有对象层测试文件 [py-list-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/objects/py-list-unittest.cc#L20-L208)，其中已覆盖：

  * `PopTagged()` 基本行为

  * `RemoveByIndex()`

  * `Clear()`

* 当前已有 GC 层 list 回归测试 [gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc#L81-L145)，但尚未覆盖“逻辑删除后尾槽引用是否清理”这一点。

### 3. 当前最适合落点的测试位置

* 对象层语义测试：

  * [py-list-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/objects/py-list-unittest.cc)

* GC 层保活/搬迁回归：

  * [gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc)

## Proposed Changes

### 1. 修改 `src/objects/py-list.cc`

文件：

* [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc)

计划修改点：

#### 1.1 修复 `PopTagged()`

当前：

* 先 `--length_`

* 直接返回原尾元素

计划：

* 先读取尾元素保存到局部变量

* 将原尾槽显式写为 `Tagged<PyObject>::null()`

* 再更新 `length_`

* 返回原尾元素

原因：

* 保证从 list 逻辑弹出的对象不会继续被底层数组引用。

#### 1.2 修复 `RemoveByIndex()`

当前：

* 通过前移覆盖删除位置

* 最后只 `--length_`

计划：

* 保持现有前移逻辑

* 在前移完成后，将原最后一个有效槽位显式写空

* 再减少 `length_`

原因：

* 保证删除后尾槽不再残留旧对象引用。

#### 1.3 修复 `Clear()`

当前：

* 只把 `length_` 设为 0

计划：

* 遍历当前有效区间 `[0, length_)`

* 将每个槽位显式清空

* 最后把 `length_` 置 0

原因：

* 彻底断开 list 对当前所有逻辑元素的引用。

实现注意事项：

* 该路径不需要变更公开 API。

* 当前堆模型下，使用 `array()->Set(..., Tagged<PyObject>::null())` 即可。

* 保持代码风格与现有 `PyList` 实现一致，不引入额外抽象层。

### 3. 新增 `test/unittests/heap/gc-unittest.cc` 回归

文件：

* [gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc)

计划新增 GC 回归测试：

#### 3.1 删除后对象不再被 list 保活

测试思路：

* 构造 list，放入若干新生代对象；

* 删除其中一部分对象（通过 `PopTagged` / `RemoveByIndex` / `Clear` 中至少覆盖最关键路径）；

* 只保留对 list 和仍应存活对象的 handle；

* 制造 GC 压力并触发 `CollectGarbage()`；

* 验证：

  * list 语义仍正确；

  * 仍在 list 内的对象可正常访问；

  * 对于已删除对象，不再通过 list 的底层残留引用可达。

可执行的验证形式：

* 重点检查 list 的尾槽/有效区外槽位已为空；

* 结合 GC 前后地址变化与行为断言，证明“剩余有效元素仍正确、无效元素不再由 list 持有”。

说明：

* 当前测试框架不适合直接断言“对象一定被回收”，因此计划采用更稳妥的白盒 + GC 行为组合验证，而不是依赖对象析构或计数器。

## Assumptions & Decisions

* 本次范围只做 `PyList` 尾槽清理，不顺带治理 `PopTagged/GetTagged` API 形状，也不处理 `extend(self)` 风险。

* 本次修复目标是“消除额外保活”，不是做 full GC discipline 重构。

* 当前 minor GC 模型下，清空 `FixedArray` 槽位即可切断 list 对对象的引用。

* 允许在对象层单测中做适度白盒验证，因为这里关注的是容器内部存储语义，而不是仅 Python 层可见行为。

## Verification

实施后按以下顺序验证：

1. 编译 `ut` 目标，确保对象层与 heap 层单测都能通过编译。
2. 运行 `PyListTest.*`，确认对象层行为无回归。
3. 运行 `GcTest.*` 中相关用例，确认 GC 行为无回归。
4. 视编译成本决定是否运行完整 `ut.exe`，若本轮改动稳定则建议跑完整回归。

建议最少验证集：

* `PyListTest.*`

* `GcTest.CopyGcTestForPyList`

* 新增的 PyList 尾部引用清理 GC 回归测试

