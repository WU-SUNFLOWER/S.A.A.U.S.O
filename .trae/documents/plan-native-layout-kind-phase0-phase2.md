# NativeLayoutKind Phase 0\~2 实施计划

## Summary

* 目标：只推进既有 roadmap 中的 **Phase 0 \~ Phase 2**，把一批适合的内建类型纳入 `NativeLayoutKind`，并将对应 `IsXxx` checker 从 klass 单例地址比较迁移为基于 `native_layout_kind` 的比较。

* 本轮的核心约束：

  * 保持 `NativeLayoutKind = 物理布局标签`

  * 不引入新的 `IsXxxExact` API

  * 不进入 `PyFunction / NativeFunction` family-kind 议题

  * 不改变 `PyString / PyList / PyTuple / PyDict` 现有 like/exact 语义

* 本轮的目标是“建立一套可扩展的 layout-kind checker 基线”，不是一次性清空全仓所有 `Isolate::Current()`。

## Current State Analysis

### 1. NativeLayoutKind 当前只覆盖四类继承型容器

* 当前枚举仅有：

  * `kPyObject`

  * `kList`

  * `kDict`

  * `kTuple`

  * `kString`

  * 见 [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h#L30-L36)

* 这与当前支持用户继承的 heap builtin 完全一致：

  * `PyString / PyList / PyTuple / PyDict`

  * 见 [object-type-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-type-list.h#L12-L16)

### 2. 当前 checker 已经形成三层语义

* 第一层：容器 like checker，已按 layout kind 判定：

  * `IsPyString / IsPyList / IsPyTuple / IsPyDict`

  * 见 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L73-L87)

* 第二层：容器 exact checker，已作为已有 API 存在：

  * `IsPyStringExact / IsPyListExact / IsPyTupleExact / IsPyDictExact`

  * 见 [object-checkers.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.h#L32-L40)

* 第三层：其余非容器 checker 仍大量依赖：

  * `PyObject::GetKlass(object) == XxxKlass::GetInstance(Isolate::Current())`

  * 见 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L43-L67)

### 3. Phase 2 候选类型的成熟度

* 以下类型已经具备“独立、稳定的物理布局”，且当前 `IsXxx` 语义本身就是 exact，适合迁移为 exact-by-kind：

  * `PyFloat` [py-float.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-float.h)

  * `PyTypeObject` [py-type-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.h)

  * `PyCodeObject` [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h)

  * `FixedArray` [fixed-array.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.h)

  * `Cell` [cell.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/cell.h)

  * `MethodObject` [py-function.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.h#L95-L112)

  * `PyListIterator` [py-list-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.h)

  * `PyTupleIterator` [py-tuple-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.h)

  * `PyDictKeys / PyDictValues / PyDictItems / PyDictKeyIterator / PyDictItemIterator / PyDictValueIterator` [py-dict-views.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.h)

* 这些类型当前大多尚未在 `PreInitialize` 中设置专属 kind，例如：

  * [py-float-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-float-klass.cc#L68-L95)

  * [py-type-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc#L45-L65)

  * [py-code-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object-klass.cc#L36-L47)

  * [fixed-array-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array-klass.cc#L26-L37)

  * [cell-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/cell-klass.cc#L27-L37)

  * [py-list-iterator-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator-klass.cc#L58-L70)

  * [py-dict-views-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views-klass.cc#L78-L91)

### 4. 本轮明确排除的范围

* `PySmi`：不是 heap object，无法纳入 klass 上的 layout kind 路径。

* `PyModule`：当前没有超出 `PyObject` 的独立物理布局，见 [py-module.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-module.h#L12-L18)。

* `PyBaseException`：当前显式保留 `kPyObject` 布局，语义字段仍经 `__dict__` 承载，见 [py-base-exception-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-base-exception-klass.cc#L69-L76)。

* `PyBoolean / PyNone`：主要是 singleton / identity 语义，不纳入本轮。

* `PyFunction / NativeFunction`：保留到后续 family-kind 议题，不纳入本轮。

## Proposed Changes

### Phase 0：冻结语义矩阵与命名规则

* 修改文件：

  * `src/objects/object-checkers.h`

  * `src/objects/object-checkers.cc`

  * `.trae/documents/plan-native-layout-kind-checker-roadmap.md`

* 具体工作：

  * 在 `object-checkers.h` 中补强注释，明确本轮只存在三类规则：

    * 容器 like：`IsPyString / IsPyList / IsPyTuple / IsPyDict`

    * 非继承型 exact-by-kind：本轮新增 kind 但不新增 `Exact` API

    * identity / special-case：`IsPySmi / IsPyTrue / IsPyFalse / IsPyNone / IsHeapObject / IsGcAbleObject`

  * 在 roadmap 文档中回写本轮的实际分组与边界，确保执行期不会摇摆到“把 kind 误用成 type identity”。

* 目的：

  * 先把语义边界写死，再进入代码变更，避免 Phase 2 中途扩 scope。

### Phase 1：规范化现有容器 checker 基线

* 修改文件：

  * `src/objects/object-checkers.h`

  * `src/objects/object-checkers.cc`

  * `test/unittests/objects/py-object-unittest.cc`

* 具体工作：

  * 不改变现有四个容器 checker 的对外 API 和语义：

    * `IsPyString / IsPyList / IsPyTuple / IsPyDict` 继续表示 like

    * 现有 `IsPyStringExact / IsPyListExact / IsPyTupleExact / IsPyDictExact` 继续保留，仅用于既有边界

  * 在 `object-checkers.cc` 中统一抽象一个内部 helper：

    * `IsNativeLayoutKind(object, expected_kind)`

    * 容器 checker 全部收敛到该 helper

  * 在 `py-object-unittest.cc` 中补强已有 like/exact 场景断言，确保容器基线成为 Phase 2 的参考模板。

* 目的：

  * 先把当前容器逻辑整理成标准范式，供后续非继承型 exact-by-kind 复用。

### Phase 2：新增第一批 exact-by-kind 类型

* 修改文件：

  * `src/objects/klass.h`

  * `src/objects/object-checkers.cc`

  * `src/objects/py-float-klass.cc`

  * `src/objects/py-type-object-klass.cc`

  * `src/objects/py-code-object-klass.cc`

  * `src/objects/fixed-array-klass.cc`

  * `src/objects/cell-klass.cc`

  * `src/objects/py-function-klass.cc`

  * `src/objects/py-list-iterator-klass.cc`

  * `src/objects/py-tuple-iterator-klass.cc`

  * `src/objects/py-dict-views-klass.cc`

* 枚举扩展方案：

  * 在 `NativeLayoutKind` 中新增以下值：

    * `kFloat`

    * `kTypeObject`

    * `kCodeObject`

    * `kFixedArray`

    * `kCell`

    * `kMethodObject`

    * `kListIterator`

    * `kTupleIterator`

    * `kDictKeys`

    * `kDictValues`

    * `kDictItems`

    * `kDictKeyIterator`

    * `kDictItemIterator`

    * `kDictValueIterator`

* 初始化规则：

  * 在上述各自 `PreInitialize` 中，统一增加：

    * `set_native_layout_kind(NativeLayoutKind::kXxx);`

    * `set_native_layout_base(PyObjectKlass::GetInstance(isolate));`

  * `MethodObjectKlass::PreInitialize` 放在 [py-function-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function-klass.cc#L226) 中处理。

* checker 重构规则：

  * 将 `object-checkers.cc` 中对应这批类型的 klass 比较实现，改写为：

    * `return IsNativeLayoutKind(object, NativeLayoutKind::kXxx);`

  * 这里的“kind 比较”仍代表 exact 语义，因为这些类型当前不支持 Python 用户类继承，也不新增 `IsXxxExact` API。

  * `IsPyFunction / IsNormalPyFunction / IsNativePyFunction` 保持现状，不在本轮迁移。

  * `IsPyTrue / IsPyFalse / IsPyNone / IsPySmi` 保持现状，不在本轮迁移。

* 目的：

  * 在不增加 API 表面积的前提下，让第一批非继承型 builtin checker 摆脱 `Isolate::Current()`。

## Test Plan

### 1. 复用现有对象层单测，最小化构建改动

* 不新增新的 `IsXxxExact` API，也不新增新的测试 target。

* 以“就近补断言”为原则，优先修改现有单测文件：

  * `test/unittests/objects/py-object-unittest.cc`

  * `test/unittests/objects/py-float-unittest.cc`

  * `test/unittests/objects/cell-unittest.cc`

  * `test/unittests/objects/py-dict-unittest.cc`

  * `test/unittests/objects/py-tuple-unittest.cc`

  * 如有必要，补充 `test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc`

### 2. 断言重点

* 容器基线：

  * `IsPyList / IsPyDict / IsPyTuple / IsPyString` 的 like 行为不回退

  * 现有容器 exact API 行为不回退

* 第一批 exact-by-kind 类型：

  * `IsPyFloat`

  * `IsPyTypeObject`

  * `IsPyCodeObject`

  * `IsFixedArray`

  * `IsCell`

  * `IsMethodObject`

  * `IsPyListIterator / IsPyTupleIterator`

  * `IsPyDictKeys / Values / Items / KeyIterator / ItemIterator / ValueIterator`

* 回归重点：

  * checker 行为与修改前一致

  * 不因 `kind` 迁移导致误把共享 `kPyObject` 布局类型判断为特殊类型

## Assumptions & Decisions

* 决策：只做 roadmap 中的 Phase 0 \~ Phase 2，不进入 Phase 3 function family。

* 决策：不引入新的 `IsXxxExact` API；仅保留当前四大容器已有的 exact API。

* 决策：Phase 2 中新增的 kind，只服务于“非继承型 exact-by-kind checker”，不改变这些 API 的外部语义。

* 决策：所有新增 kind 的类型，都在 `PreInitialize` 中显式写入 `native_layout_kind` 和 `native_layout_base`，避免依赖默认值。

* 决策：`kPyObject` 继续稳定表示“无特殊 native layout”，因此 `PyModule / PyBaseException / PyNone` 等共享布局类型不纳入本轮。

## Verification Steps

* 静态检查：

  * 检索 `object-checkers.cc` 中 Phase 2 覆盖类型是否仍残留 `XxxKlass::GetInstance(Isolate::Current())`

  * 检索对应 `PreInitialize` 是否已设置新的 `NativeLayoutKind`

* diagnostics：

  * 确保新增枚举值与 checker 重构后无签名/符号错误

* 构建验证：

  * `ninja -C out/debug ut`

* 定向回归：

  * `py-object`

  * `py-float`

  * `cell`

  * `py-dict`

  * `py-tuple`

  * `interpreter-builtins-bootstrap`

* 完成标准：

  * Phase 2 覆盖类型的 checker 不再依赖 `Isolate::Current()`

  * 不新增任何新的 `IsXxxExact` API

  * 容器 like/exact 语义保持不变

  * 第一批 exact-by-kind 类型的 checker 行为与迁移前一致

