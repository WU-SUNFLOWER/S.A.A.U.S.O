# PyString 剩余 Current 清理优先级计划

## Summary

* 目标：按你指定的优先级，继续推进 `PyString` 中剩余 4 组 helper 的显式 `Isolate*` 收口，彻底清掉 [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc) 里这批对象层内部的 `Isolate::Current()` 直取点。

* 优先级严格按以下顺序执行：

  * 第一优先：`FromStdString`

  * 第二优先：`Slice`

  * 第三优先：`FromInt / FromDouble` 连同 `FromPySmi / FromPyFloat`

* 本轮只做 API 收口与直接调用链同步，不扩展到 `HandleScope` 基建治理，也不改变 `Slice` 现有“含尾端”的区间语义。

## Current State Analysis

### 1. 当前 PyString API 基线

* `PyString::New / Clone / Append` 已完成显式 `Isolate*` 化，见 [py-string.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.h#L24-L43) 与 [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L37-L67)。

* 目前剩余的直接 `Isolate::Current()` 都集中在 `py-string.cc` 内部 helper：

  * `FromInt` [py-string.cc:L77-L84](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L77-L84)

  * `FromDouble` [py-string.cc:L92-L99](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L92-L99)

  * `FromStdString` [py-string.cc:L102-L105](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L102-L105)

  * `Slice` [py-string.cc:L199-L215](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L199-L215)

### 2. 第一优先：FromStdString 的现状

* `FromStdString` 只有单跳构造逻辑，改造最直接：

  * [py-string.cc:L102-L105](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L102-L105)

* 当前主要调用点集中在 runtime 文本展示路径，且调用方都已持有 `isolate`：

  * [runtime-py-list.cc:L16-L32](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-list.cc#L16-L32)

  * [runtime-py-tuple.cc:L17-L36](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-tuple.cc#L17-L36)

  * [runtime-py-dict.cc:L257-L286](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-dict.cc#L257-L286)

  * [runtime-py-string.cc:L256-L260](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-string.cc#L256-L260)

  * [runtime-py-function.cc:L22-L40](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-function.cc#L22-L40)

  * [runtime-py-type-object.cc:L93-L103](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-type-object.cc#L93-L103)

* 结论：调用面集中、语义简单、风险低，适合作为第一批收尾点。

### 3. 第二优先：Slice 的现状

* `Slice` 当前语义为“含尾端”，实现内部只剩一个 `PyString::New(Isolate::Current(), sliced_length)`：

  * [py-string.cc:L199-L215](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L199-L215)

* 当前主要调用点集中在 import/module 链路：

  * [module-importer.cc:L82-L122](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc#L82-L122)

  * [module-importer.cc:L213-L221](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc#L213-L221)

  * [module-loader.cc:L94-L103](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc#L94-L103)

  * [module-loader.cc:L181-L195](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc#L181-L195)

  * [module-name-resolver.cc:L137-L143](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc#L137-L143)

  * [builtin-random-module.cc:L290-L300](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-random-module.cc#L290-L300)

* 结论：`Slice` 的 isolate 透传本身不复杂，但会触及 import 主链路，因此需要把语义保护与回归验证放在更高优先级。

### 4. 第三优先：数值转字符串接口族现状

* `FromPySmi` 和 `FromPyFloat` 只是薄封装：

  * [py-string.cc:L72-L74](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L72-L74)

  * [py-string.cc:L87-L89](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L87-L89)

* 真实调用链分别落在 `int/float` 的 `Virtual_Repr`：

  * [py-smi-klass.cc:L106-L113](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-smi-klass.cc#L106-L113)

  * [py-float-klass.cc:L183-L190](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-float-klass.cc#L183-L190)

* 对象层测试已覆盖整数/浮点字符串化行为：

  * [py-string-unittest.cc:L206-L221](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/objects/py-string-unittest.cc#L206-L221)

* 结论：这一组调用面小，但应成组推进，避免只把桥接从 `FromInt / FromDouble` 平移到 `FromPySmi / FromPyFloat`。

## Proposed Changes

### P1：先收口 FromStdString

* 修改文件：

  * `src/objects/py-string.h`

  * `src/objects/py-string.cc`

  * `src/runtime/runtime-py-list.cc`

  * `src/runtime/runtime-py-tuple.cc`

  * `src/runtime/runtime-py-dict.cc`

  * `src/runtime/runtime-py-string.cc`

  * `src/runtime/runtime-py-function.cc`

  * `src/runtime/runtime-py-type-object.cc`

* 具体改法：

  * 将 `PyString::FromStdString(std::string source)` 改为 `PyString::FromStdString(Isolate* isolate, std::string source)`。

  * 所有 runtime 调用点直接传已有的 `isolate`，不保留旧签名兼容壳。

* 目的：

  * 先清掉最简单的一类 `Current()` 直取点，并把 repr/文本展示链路统一成显式 isolate 风格。

### P2：再收口 Slice

* 修改文件：

  * `src/objects/py-string.h`

  * `src/objects/py-string.cc`

  * `src/modules/module-importer.cc`

  * `src/modules/module-loader.cc`

  * `src/modules/module-name-resolver.cc`

  * `src/modules/builtin-random-module.cc`

  * `test/unittests/objects/py-string-unittest.cc`

  * 以及 import / random 相关受影响测试文件

* 具体改法：

  * 将 `Slice(self, from)` 与 `Slice(self, from, to)` 改为显式接收 `Isolate* isolate`。

  * 所有调用点机械式透传已有的 `isolate` 或 `isolate_`。

* 关键约束：

  * 本轮不改变 `Slice` 的“含尾端”语义，只做 isolate 透传。

  * 需要补强回归测试，确保 import fullname 拆段、top-level import return 语义与字符串切片行为保持不变。

### P3：最后成组收口数值转字符串接口族

* 修改文件：

  * `src/objects/py-string.h`

  * `src/objects/py-string.cc`

  * `src/objects/py-smi-klass.cc`

  * `src/objects/py-float-klass.cc`

  * `test/unittests/objects/py-string-unittest.cc`

* 具体改法：

  * 将以下接口统一改为显式 `Isolate*`：

    * `FromInt(Isolate* isolate, int64_t n)`

    * `FromDouble(Isolate* isolate, double n)`

    * `FromPySmi(Isolate* isolate, Tagged<PySmi> smi)`

    * `FromPyFloat(Isolate* isolate, Handle<PyFloat> py_float)`

  * `PySmiKlass::Virtual_Repr` 与 `PyFloatKlass::Virtual_Repr` 直接透传其已有的 `isolate`。

* 目的：

  * 保证这一组接口内部不再出现新的隐式桥接残留，完成对象层数字字符串化的风格统一。

### P4：验证与回归

* 静态验证：

  * 检索 `src/objects/py-string.cc` 是否已不再出现 `Isolate::Current()`。

  * 检索 `PyString::FromStdString(`、`PyString::Slice(`、`PyString::FromInt(`、`PyString::FromDouble(`、`PyString::FromPySmi(`、`PyString::FromPyFloat(` 的调用是否全部切到新签名。

  * 运行 diagnostics 检查是否存在漏传 `isolate`。

* 构建验证：

  * 构建 `ut`

* 定向回归：

  * `test/unittests/objects/py-string-unittest.cc`

  * import 相关解释器/模块测试，重点覆盖 `pkg.sub`、`fromlist`、top-level return 语义

  * 与字符串展示相关的 `repr/print` 路径测试

  * 必要时补跑 `float/int` 字符串化相关测试，确认输出格式未漂移

## Assumptions & Decisions

* 决策：严格按“`FromStdString` → `Slice` → 数值转换接口族\`”推进，不交叉重排优先级。

* 决策：本轮不保留旧签名兼容层，继续沿用前面 PyDict / PyList / PyString API 收口时的一步到位策略。

* 决策：本轮不推进 `HandleScope` 显式 isolate 化，不将任务扩大为全局 `Isolate::Current()` 治理。

* 决策：`Slice` 只做 isolate 透传，不改现有区间语义；若验证中暴露既存边界问题，仅在不改变本轮目标的前提下做最小必要修补。

* 决策：不扩展到 `ModuleUtils::NewPyString`、`BuiltinModuleRegistry` 等额外 helper；它们属于下一轮潜在收尾项，不并入本计划。

## Verification

* 完成标准：

  * [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc) 内上述 4 组 helper 的 `Isolate::Current()` 全部清零。

  * `PyString` 剩余对象层 helper 与容器风格一致，调用方显式透传 `Isolate*`。

  * 直接调用链构建通过，无新增 diagnostics。

  * 相关对象层、repr/print、import/module 定向回归通过。

