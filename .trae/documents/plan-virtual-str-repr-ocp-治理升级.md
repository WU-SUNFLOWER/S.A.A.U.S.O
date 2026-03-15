# Virtual\_Str/Virtual\_Repr 架构治理计划（OCP 对齐）

## Summary

* 你的观察是合理的：当前 `__str__/__repr__` 主要集中在 runtime 默认路径（`runtime-py-string.cc` + `runtime-py-repr.cc`）并由 `PyObjectKlass::Generic_Str/Generic_Repr` 统一入口兜底，不利于按类型开闭扩展。

* 本计划目标是把“类型专属文本语义”下沉到各内建类型 `Klass` 的 `Virtual_Str/Virtual_Repr`，并绑定到对应 vtable slot；runtime 仅保留通用编排能力与少量共享 helper。

* 在不回退当前行为的前提下分阶段迁移：先补齐 slot 覆盖，再收缩 `Runtime_NewStrDefault/Runtime_NewReprDefault` 的类型分支，最终让新增类型无需改 central switch。

## Current State Analysis

* 当前 `repr_/str_` 显式赋值仅在 `PyObjectKlass`，其余类型主要继承该默认实现：

  * [py-object-klass.cc#L57-L59](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L57-L59)

  * [klass-vtable.cc#L32-L63](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc#L32-L63)

* 当前默认入口：

  * `Generic_Repr/Generic_Str`： [py-object-klass.cc#L355-L367](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L355-L367)

  * `Runtime_NewReprDefault`： [runtime-py-repr.cc#L173-L233](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-repr.cc#L173-L233)

  * `Runtime_NewStrDefault`： [runtime-py-string.cc#L222-L253](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-string.cc#L222-L253)

* 现状问题：

  * 类型文本语义扩展需要修改 central 分支，违反 OCP；

  * `runtime-py-repr.cc` 承担了大量“类型特化决策”而非纯 runtime 编排；

  * 内建方法安装层（`builtins-py-*-methods`）对 `__str__/__repr__` 暴露普遍缺失，槽位语义与 Python 层可见方法未完全一致。

## Proposed Changes

### 1) 在对象系统引入类型级 Virtual\_Str/Virtual\_Repr

* 目标文件：

  * `src/objects/py-list-klass.h/.cc`

  * `src/objects/py-tuple-klass.h/.cc`

  * `src/objects/py-dict-klass.h/.cc`

  * `src/objects/py-string-klass.h/.cc`

  * `src/objects/py-smi-klass.h/.cc`

  * `src/objects/py-float-klass.h/.cc`

  * `src/objects/py-oddballs-klass.h/.cc`（bool/None）

  * `src/objects/py-type-object-klass.h/.cc`

  * `src/objects/py-function-klass.h/.cc`（function/native-function/method）

* 变更方式：

  * 为上述 `Klass` 增加 `Virtual_Repr`、`Virtual_Str` 声明与定义；

  * 在 `PreInitialize()` 中绑定 `vtable_.repr_` 与 `vtable_.str_`；

  * 语义约束：

    * `str` 返回 `PyString`；

    * `repr` 返回 `PyString`，容器递归元素走 `PyObject::Repr`；

    * 错误传播遵循 `MaybeHandle` + pending exception。

* 预期收益：

  * 新增类型只需在本类型实现自己的 `Virtual_Str/Virtual_Repr`，无需改 runtime 中央分支。

### 2) 收缩 runtime 默认分支为“最小兜底”

* 目标文件：

  * `src/runtime/runtime-py-string.cc`

  * `src/runtime/runtime-py-repr.cc`

* 变更方式：

  * `Runtime_NewStrDefault` 仅保留极小公共 fallback（必要时委托 `Runtime_NewRepr`）；

  * `Runtime_NewReprDefault` 移除针对 list/tuple/dict/type/function 等具体类型的分发逻辑，仅保留 object 级 fallback；

  * 迁移后，类型细节由 vtable slot 决定，runtime 只负责协议校验与公共包装。

### 3) 对齐 Python 层可见方法（可选但建议在同轮完成）

* 目标文件：

  * `src/builtins/builtins-py-object-methods.h/.cc`

  * `src/builtins/builtins-py-list-methods.h/.cc`

  * `src/builtins/builtins-py-dict-methods.h/.cc`

  * `src/builtins/builtins-py-tuple-methods.h/.cc`

  * `src/builtins/builtins-py-string-methods.h/.cc`

  * `src/builtins/builtins-py-type-object-methods.h/.cc`

* 变更方式：

  * 逐步补齐 `__str__`/`__repr__` 的 methods 安装，使 Python 层 introspection 与 slot 行为一致。

### 4) 保持 trampoline 覆盖路径不变

* 目标文件：

  * `src/objects/klass-vtable.cc`

  * `src/objects/klass-vtable-trampolines.cc`

* 策略：

  * 不改 `UpdateOverrideSlots` 与 `KlassVtableTrampolines::Str/Repr` 的机制，继续支持用户类覆写 `__str__/__repr__`。

### 5) 构建与测试增量

* 目标文件：

  * `test/unittests/interpreter/native-print-unittest.cc`

  * `test/unittests/interpreter/interpreter-custom-class-unittest.cc`

  * `test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc`

* 新增/扩展覆盖：

  * 内建容器与标量 `str/repr` 输出一致性；

  * `print(x)` 走 `str`，`repr(x)` 走 `repr`；

  * 自定义类覆盖 `__str__/__repr__` 仍通过 trampoline 生效。

## Assumptions & Decisions

* 决策 1：本轮优先覆盖 Python 语言层可见且高频的内建类型（`object/int/float/bool/None/str/list/tuple/dict/type/function/native-function/method`），不优先治理 `Cell/FixedArray` 等内部基建类型。

* 决策 2：保持现有异常与返回约定，不改变 `MaybeHandle` 契约。

* 决策 3：不引入新的 descriptor 语义扩展，仅做 `str/repr` 分层治理。

* 决策 4：`Runtime_NewStr/Runtime_NewRepr` 作为统一入口保留，方便 `print/repr builtin` 与其他 runtime 调用点复用。

## Verification Steps

1. 构建验证

   * `gn gen out/ut --args="is_debug=true"`

   * `ninja -C out/ut ut`
2. 定向测试

   * `ut.exe --gtest_filter="NativePrintTest.*:BuiltinsBootstrapTest.BuiltinsContainCoreEntries:BasicInterpreterTest.CustomClassStrAndReprWorkForPrintAndBuiltinRepr"`
3. 语义回归检索

   * 确认 `runtime-py-repr.cc` 不再承载多类型 central 分发；

   * 确认目标 `Klass` 中存在并绑定 `Virtual_Str/Virtual_Repr`；

   * 确认 `print/repr` builtin 测试仍全绿。

