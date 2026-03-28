# NativeLayoutKind 扩展与 IsXxx Checker 重构 Roadmap

## Summary

* 目标：评估哪些内建类型适合纳入 `NativeLayoutKind`，并给出一份可执行的 checker 重构路线，使 `IsXxx` 系列尽量从“依赖 klass 单例地址比较”迁移到“基于对象 klass 元信息的稳定字段比较”。

* 核心判断：这个方向是合理的，但前提是**保持** **`NativeLayoutKind = 物理布局标签`**，不要把它直接扩成“全体内建类型身份枚举”。

* 因此，本次 roadmap 的原则是：

  * `NativeLayoutKind` 只覆盖“拥有独立物理布局，且值得被统一建模”的 heap builtin。

  * `IsXxx` 是否改造，不只看“有没有独立类型名”，还要看该类型是否有稳定布局、是否允许 like 语义、是否已有 exact/identity 特殊语义。

  * `IsXxxExact`、`IsPyTrue/False`、`IsPySmi` 等特殊判定继续保留各自边界，不被 layout kind 一刀切吞并。

  * 对非继承型 heap builtin，优先采用“不新增额外 `IsXxxExact` API，只把现有 `IsXxx` 实现迁为 exact-by-kind”的策略。

## Current State

### 1. NativeLayoutKind 当前语义

* 当前 `NativeLayoutKind` 只定义了 5 个值：

  * `kPyObject`

  * `kList`

  * `kDict`

  * `kTuple`

  * `kString`

  * 见 [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h#L30-L36)

* `Klass` 上同时维护：

  * `native_layout_kind`

  * `native_layout_base`

  * `instance_has_properties_dict`

  * 见 [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h#L59-L74) 与 [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h#L123-L131)

* 动态 Python 类创建时，会把 `kPyObject` 视为“无特殊 native 布局”，并只允许一种特殊布局基类参与继承：

  * 见 [runtime-reflection.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-reflection.cc#L52-L101)

### 2. Checker 当前语义分层

* 当前 `object-checkers` 已经有明确的语义分层：

  * `IsPyString / IsPyList / IsPyTuple / IsPyDict`：like，按 layout kind 判定

  * `IsPyStringExact / IsPyListExact / IsPyTupleExact / IsPyDictExact`：exact，按 klass identity 判定

  * 其余大量 `IsXxx`：仍按 `klass == XxxKlass::GetInstance(Isolate::Current())`

  * 特殊对象：`IsPySmi / IsPyTrue / IsPyFalse / IsHeapObject / IsGcAbleObject`

  * 见 [object-checkers.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.h#L32-L40) 与 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L43-L67)

### 3. 当前支持用户继承的 heap builtin

* 仅有四类继承型 heap builtin：

  * `PyString`

  * `PyList`

  * `PyTuple`

  * `PyDict`

  * 见 [object-type-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-type-list.h#L12-L16)

* 这与当前 `NativeLayoutKind` 的枚举范围基本一致，说明现阶段它主要服务于“可继承内建容器的布局建模”。

## 评估结论

### A. 立即适合纳入 layout kind，并适合做 checker 重构的类型

* 这组类型已经满足全部条件：

  * 有稳定独立物理布局

  * 已参与 native layout 继承语义

  * 已有 like/exact 分层需求

  * 已有对应 `New*Like` 或等价分配模式

* 类型清单：

  * `PyString` [py-string.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.h)

  * `PyList` [py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h)

  * `PyTuple` [py-tuple.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple.h)

  * `PyDict` [py-dict.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.h)

* 对应建议：

  * 保持 `IsPyXxx = like`

  * 保持 `IsPyXxxExact = exact`

  * 将所有调用点里的“容器 like 语义”统一收敛到 `native_layout_kind` 判断，不再混入 klass 地址判断

* 这组类型本轮不是“新增纳入”，而是“确立为 checker 重构的规范基线”。

### B. 适合新增 layout kind，并把 `IsXxx` 改为 exact-by-kind 的类型

* 这组类型的共同特征：

  * 是 heap object

  * 有独立、稳定、可识别的 C++ 物理字段布局

  * 当前不支持用户继承，不需要 like 语义

  * `IsXxx` 仍应保持 exact 语义，但实现可以从 klass 地址比较迁移为 kind 比较

* 推荐纳入的第一批：

  * `PyFloat` [py-float.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-float.h)

  * `PyTypeObject` [py-type-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.h)

  * `PyCodeObject` [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h)

  * `FixedArray` [fixed-array.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.h)

  * `Cell` [cell.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/cell.h)

  * `MethodObject` [py-function.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.h#L95-L112)

  * `PyListIterator` [py-list-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.h)

  * `PyTupleIterator` [py-tuple-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.h)

  * `PyDictKeys / PyDictValues / PyDictItems` [py-dict-views.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.h)

  * `PyDictKeyIterator / PyDictItemIterator / PyDictValueIterator` [py-dict-views.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.h)

* 对应建议：

  * 为这些类型新增 `NativeLayoutKind::kXxx`

  * 在各自 `PreInitialize` 中设置 `set_native_layout_kind(...)`

  * 保持 `IsXxx` 的**外部语义不变**，但把实现改成 `kind == kXxx`

* 这样做的收益是：

  * 去掉 checker 对 `Isolate::Current()` 的依赖

  * 不改变现有 like/exact 语义模型

  * 对调用方几乎透明

### C. 条件成立后适合纳入 layout kind 的“家族型”类型

* 这组类型不适合简单按“一个 klass 一个 kind”硬切，但可以按**布局家族**建模。

* 重点候选：

  * `PyFunction`

  * `NativeFunctionKlass`

* 依据：

  * 两者共享 `PyFunction` 物理对象形态，见 [py-function.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.h#L15-L93)

  * 当前 `IsPyFunction` 也是组合语义，见 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L92-L107)

* 建议：

  * 若后续引入 `NativeLayoutKind::kFunction`，则 `IsPyFunction` 可重构为 kind-family 判断

  * 但 `IsNormalPyFunction` / `IsNativePyFunction` 仍需保持 klass exact 判定

* 注意：

  * 这个方向需要先确认 `PyFunctionKlass` 与 `NativeFunctionKlass` 的 `instance_has_properties_dict` 差异不会影响我们对“layout 家族”的定义

### D. 当前不适合纳入 layout kind 的类型

* `PySmi`

  * 不是 heap object，没有 klass header，无法纳入 klass 上的 layout kind 框架

* `PyModule`

  * 当前物理布局等同于 `PyObject`，见 [py-module.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-module.h#L12-L18)

* `PyBaseException`

  * 当前仍显式走 `kPyObject`，语义字段靠 `__dict__` 承载，见 [py-base-exception-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-base-exception-klass.cc#L69-L76)

* `PyNone`

  * 本身没有额外布局价值；其 checker 价值主要在 singleton identity，而非 layout

* 这组类型如果强行纳入，只会把“布局标签”退化成“身份标签”，破坏 `kPyObject` 作为“无特殊 native 布局”的语义。

## 设计原则

### 原则 1：坚持 `NativeLayoutKind = 物理布局标签`

* 不把 layout kind 直接定义成“全体 builtin type id”。

* 否则：

  * `runtime-reflection.cc` 中“多特殊布局冲突”的语义会被污染

  * `kPyObject` 将不再稳定表示“无特殊 native layout”

### 原则 2：checker 改造分为 like 与 exact 两条线

* 对继承型 builtin：

  * `IsXxx` 走 like(kind)

  * `IsXxxExact` 走 exact(klass)

* 对非继承型但有独立布局的 builtin：

  * `IsXxx` 仍是 exact 语义

  * 但实现可改成 exact-by-kind

### 原则 3：identity/singleton 语义不并入 layout kind

* `IsPyTrue / IsPyFalse / IsPyNone` 继续保留 identity 路径

* 这类 API 的本质是对象身份，不是布局兼容

### 原则 4：不能只改 checker，必须同步布局元信息初始化

* 每新增一个 `NativeLayoutKind::kXxx`，都必须同步：

  * `klass.h` 枚举

  * 对应 `Klass::PreInitialize`

  * 必要的分配断言/工厂路径

  * 对应 checker 单测

## Roadmap

### Phase 0：冻结语义边界

* 输出一份“类型分组矩阵”，明确每种 builtin 属于以下哪一类：

  * inheritable-like

  * exact-by-kind

  * family-kind

  * identity/shared-layout

* 需要覆盖文件：

  * [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h)

  * [object-type-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-type-list.h)

  * [object-checkers.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.h)

  * [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc)

### Phase 1：规范化现有容器 like checker

* 范围：

  * `PyString / PyList / PyTuple / PyDict`

* 目标：

  * 全仓确认容器型 `IsPyXxx` 均只依赖 `native_layout_kind`

  * 关键 exact 场景继续走 `IsPyXxxExact`

* 这是后续扩展的语义模板。

### Phase 2：新增第一批 exact-by-kind 类型

* 目标类型：

  * `PyFloat`

  * `PyTypeObject`

  * `PyCodeObject`

  * `FixedArray`

  * `Cell`

  * `MethodObject`

  * `PyListIterator`

  * `PyTupleIterator`

  * `PyDictKeys / Values / Items`

  * `PyDictKeyIterator / ItemIterator / ValueIterator`

* 具体工作：

  * 扩展 `NativeLayoutKind` 枚举

  * 在对应 `PreInitialize` 中设置 kind

  * 将 `object-checkers.cc` 中对应宏展开项替换为 kind 比较实现

* 验收：

  * 对外 API 名称不变

  * checker 不再依赖 `Isolate::Current()`

  * 行为与现有 exact 语义一致

### Phase 3：处理 function family

* 目标：

  * 评估是否引入 `kFunction`

* 条件：

  * 明确 `PyFunction` 与 `NativeFunction` 的 layout-family 定义

  * 明确 `IsPyFunction` 与 `IsNormalPyFunction / IsNativePyFunction` 的分层职责

* 若条件不满足，则继续保留该组 checker 的 klass-based 实现。

### Phase 4：清理剩余 klass-based checker

* 逐项审查剩余类型：

  * `PyBoolean`

  * `PyNone`

  * `PyModule`

  * `PyBaseException`

* 目标不是强制改造，而是明确：

  * 哪些永久保留 klass/identity 判定

  * 哪些未来若分配出独立布局后再进入 layout kind 体系

## File Plan

### 必改文件

* `src/objects/klass.h`

  * 扩展 `NativeLayoutKind` 枚举

* `src/objects/object-checkers.cc`

  * 把适配类型迁到 kind 比较

* 各 builtin klass 文件

  * 在 `PreInitialize` 中补 `set_native_layout_kind(...)`

### 高概率涉及文件

* `src/objects/object-checkers.h`

  * 补充注释，明确 like / exact / family / identity 规则

* `src/runtime/runtime-reflection.cc`

  * 复查新增 kind 后 `multiple bases have instance lay-out conflict` 的行为是否仍正确

* `src/heap/factory.cc`

  * 检查新增 kind 是否要求补强 `New*Like` 或 layout 断言

## Verification

* 静态验证：

  * 检索 `object-checkers.cc` 中哪些 `IsXxx` 仍依赖 `XxxKlass::GetInstance(Isolate::Current())`

  * 检索各 `PreInitialize` 是否已覆盖预期的新 kind

* 语义验证：

  * 容器子类化相关 like/exact 单测

  * `exec` / `DICT_MERGE` / `CALL_FUNCTION_EX` / `__str__` 返回值类型检查等 exact 边界测试

* 构建验证：

  * `ninja -C out/debug ut`

* 完成标准：

  * `NativeLayoutKind` 的扩展范围与物理布局语义一致

  * 一批适配 checker 不再依赖 `Isolate::Current()`

  * like / exact / identity 三类语义边界保持清晰，无行为回归
