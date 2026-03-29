# object-checkers 第五轮残留清理计划

## Summary

* 目标：核实并彻底清理 `src/objects/object-checkers.cc` 中残留的一参旧 checker API，包括用户点名的 `L77-79`、`L99-101`、`L115-117`、`L131-133`、`L147-149`、`L159-161`、`L208-210`。

* 成功标准：

  * `object-checkers.cc` 中不再保留任何 isolate-sensitive / Exact checker 的一参兼容壳。

  * 同文件内部不再依赖这些一参壳。

  * 全仓 grep 不再命中这些旧形态的实际调用或定义残留。

  * `ut` 可重新编译并通过全量单测。

## Current State Analysis

* 当前头文件 [object-checkers.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.h#L48-L71) 已只声明显式 `Isolate*` 版本的 isolate-sensitive checker 与 Exact checker；说明“对外 API 形态”已经收口。

* 但实现文件 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L71-L219) 仍残留一参兼容壳：

  * `IMPL_PY_CHECKER_BY_KLASS` 宏里仍生成 `IsXxx(Tagged<PyObject>) -> Isolate::Current()` 包装。

  * `IsPyFunction / IsNormalPyFunction / IsNativePyFunction / IsPyTrue / IsPyFalse` 仍保留一参 `Tagged<PyObject>` 包装函数。

  * `IMPL_PY_EXACT_CHECKER_BY_KLASS` 宏仍生成一参 `Tagged<PyObject>` 与一参 `Handle<PyObject>` 包装。

* 同文件内部仍有自调用依赖，例如 [object-checkers.cc:L171-L179](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc#L171-L179) 的 `IsGcAbleObject` 还在调用一参 `IsPyNone(object)`。

* 基于全仓 grep，跨文件侧当前未发现继续依赖这些一参兼容壳的调用；命中基本收敛到 `object-checkers.cc` 自身和两处测试中的显式两参调用。

## Proposed Changes

### 1. 清理 `src/objects/object-checkers.cc`

* 修改 [object-checkers.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.cc)：

  * 在 `IMPL_PY_CHECKER_BY_KLASS` 宏中删除一参 `Tagged<PyObject>` 包装，仅保留：

    * `IsXxx(Tagged<PyObject>, Isolate*)`

    * `IsXxx(Handle<PyObject>, Isolate*)`

  * 删除下列一参 `Tagged<PyObject>` 特化函数：

    * `IsPyFunction`

    * `IsNormalPyFunction`

    * `IsNativePyFunction`

    * `IsPyTrue`

    * `IsPyFalse`

  * 调整 `IsGcAbleObject`，使其改为显式调用 `IsPyNone(object, Isolate::Current())` 或等价无歧义实现，不再依赖旧壳。

  * 收紧 `IMPL_PY_CHECKER_WITH_HANDLE_ARG` 的覆盖范围，只为 truly isolate-free 的 checker 生成一参 `Handle<PyObject>` 版本；不再覆盖 `PyNone/PyModule/PyFunction/NormalPyFunction/NativePyFunction/PyTrue/PyFalse`。

  * 在 `IMPL_PY_EXACT_CHECKER_BY_KLASS` 宏中删除一参 `Tagged<PyObject>` 与一参 `Handle<PyObject>` 包装，仅保留显式 `Isolate*` 版本。

### 2. 保持 `src/objects/object-checkers.h` 不做额外扩张

* 当前 [object-checkers.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/object-checkers.h#L20-L71) 已符合目标：

  * isolate-free checker 保留一参声明。

  * isolate-sensitive checker 与四类 Exact checker 仅保留显式 `Isolate*` 声明。

* 计划执行时仅核对 header 与 `.cc` 是否重新一致；若仓库当前状态保持不变，则不需要再修改该文件。

### 3. 必要时处理编译暴露的补充点

* 如果删除 `object-checkers.cc` 中的一参实现后暴露出新的依赖点，只处理“真正由本轮删除引起”的编译错误，不做额外范围扩张。

* 预计潜在补充点仍将局限于：

  * `src/objects/object-checkers.cc` 内部自调用

  * 少量 API/测试翻译单元若仍存在未被 grep 捕获的隐式依赖

## Assumptions & Decisions

* 假设一：用户本轮目标是继续“旧 API 彻底移除”专项，而不是扩展 checker 语义或重构 checker 分类。

* 假设二：当前头文件状态是正确目标态，应以 header 为准收紧 `.cc` 实现，而不是回退 header 暴露面。

* 决策一：本轮只清理 isolate-sensitive / Exact checker 的一参兼容壳，不触碰 isolate-free checker（如 `IsPyString/IsPyList/IsPyDict/IsMethodObject` 等）的现有一参 API。

* 决策二：若内部必须保留 `Isolate::Current()`，仅允许作为“实现内部显式传参”的桥接手段出现，不再以对外一参 API 形态存在。

## Verification

* 静态核对：

  * 重新 grep `IsPyNone/IsPyModule/IsPyFunction/IsNormalPyFunction/IsNativePyFunction/IsPyTrue/IsPyFalse/IsPyStringExact/IsPyListExact/IsPyTupleExact/IsPyDictExact` 的一参形态，确认不再命中定义或调用残留。

  * 检查编辑后 `object-checkers.cc` 与 `object-checkers.h` 的声明/定义对应关系。

* 工程验证：

  * 获取诊断，确认无新增语义或声明不匹配错误。

  * 重新编译：`ninja -C out\\ut ut`

  * 运行全量单测：`out\\ut\\ut.exe`

