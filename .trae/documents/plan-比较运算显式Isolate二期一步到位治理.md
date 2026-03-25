# 比较运算显式 Isolate\* 二期一步到位治理计划

## Summary

* 目标：将 `Greater/Less/Equal/NotEqual/GreaterEqual/LessEqual/Contains` 全链路改为显式传入 `Isolate*`。

* 风格：与第一期一致，单批次完成，不引入桥接重载或双轨过渡 API。

* 约束：允许在 `klass-vtable.h` 引入“比较专用临时签名类型”，避免影响不相关虚函数 ABI；调用点优先传已有 `isolate/isolate_`，否则使用 `Isolate::Current()`；单测调用点可直接传 `isolate_`。

## Current State Analysis

* `klass-vtable` 当前比较/contains 槽位仍为无 isolate 签名 `VirtualFuncType_MaybeBool_1_2`：`greater/less/equal/not_equal/ge/le/contains`。

  * 文件：`src/objects/klass-vtable.h`

* `PyObject` 对外入口当前比较与 contains 系列仍无 isolate 形参，且 `Contains -> ContainsBool`、`Greater -> GreaterBool` 等链路都依赖旧签名。

  * 文件：`src/objects/py-object.h`、`src/objects/py-object.cc`

* `KlassVtableTrampolines` 比较/contains 签名无 isolate，函数体内部使用 `Isolate::Current()`。

  * 文件：`src/objects/klass-vtable-trampolines.h`、`src/objects/klass-vtable-trampolines.cc`

* 各内建类型 `klass` 的 `Virtual_Greater/Less/.../Contains` 仍为无 isolate 签名，分布在多个对象类型：

  * `py-smi-klass`, `py-float-klass`, `py-string-klass`, `py-list-klass`, `py-oddballs-klass`, `py-type-object-klass`, `py-tuple-klass`, `py-dict-klass`, `py-dict-views-klass`

* 上层调用点仍大量使用旧接口：

  * 解释器：`CompareOp`、`ContainsOp` 路径（`src/interpreter/interpreter-dispatcher.cc`）

  * 单测与测试工具：`test/unittests/objects/*`、`test/unittests/heap/gc-unittest.cc`、`test/unittests/test-utils.cc` 等

## Proposed Changes

### 1) vtable 比较槽位 ABI 升级（比较专用临时签名）

* 文件：`src/objects/klass-vtable.h`

* 变更：

  * 新增比较专用的临时签名类型（例如 `VirtualFuncType_MaybeBool_2_2 = Maybe<bool> (*)(Isolate*, OopHandle, OopHandle)`）。

  * 将 `greater/less/equal/not_equal/ge/le/contains` 槽位统一切换到该签名。

  * 保留其他槽位类型不变，确保影响范围只覆盖本期目标。

* 目的：满足显式 isolate 传递，同时避免无关虚函数 ABI 扰动。

### 2) PyObject 对外接口与实现升级

* 文件：`src/objects/py-object.h`、`src/objects/py-object.cc`

* 变更：

  * 对外接口改为显式 `Isolate*`：

    * `Greater/Less/Equal/NotEqual/GreaterEqual/LessEqual`

    * `GreaterBool/LessBool/EqualBool/NotEqualBool/GreaterEqualBool/LessEqualBool`

    * `Contains/ContainsBool`

  * 实现层统一透传 `isolate` 到 vtable 比较槽位。

  * 保持原有 fast-path 与异常语义，不改变比较真值规则。

* 目的：统一对象层调用约定，与第一期算数风格对齐。

### 3) trampoline 比较链路升级

* 文件：`src/objects/klass-vtable-trampolines.h`、`src/objects/klass-vtable-trampolines.cc`

* 变更：

  * `Greater/Less/Equal/NotEqual/GreaterEqual/LessEqual/Contains` 签名增加 `Isolate* isolate`。

  * 函数体内去除本地 `Isolate::Current()` 依赖，统一使用入参 isolate。

  * trampoline 内部递归调用（如 `NotEqual -> Equal`、`GreaterEqual -> Greater/Equal`、`LessEqual -> Less/Equal`）同步传递 isolate。

* 目的：打通 override/trampoline 路径的显式 isolate 契约。

### 4) 各内建类型 klass 虚函数签名与实现同步

* 文件（头文件 + 实现文件）：

  * `src/objects/py-smi-klass.h/.cc`

  * `src/objects/py-float-klass.h/.cc`

  * `src/objects/py-string-klass.h/.cc`

  * `src/objects/py-list-klass.h/.cc`

  * `src/objects/py-oddballs-klass.h/.cc`

  * `src/objects/py-type-object-klass.h/.cc`

  * `src/objects/py-tuple-klass.h/.cc`

  * `src/objects/py-dict-klass.h/.cc`

  * `src/objects/py-dict-views-klass.h/.cc`

* 变更：

  * 对应 `Virtual_Greater/Less/Equal/NotEqual/GreaterEqual/LessEqual/Contains` 统一增加 `Isolate* isolate` 入参。

  * 内部 `ASSIGN_RETURN_ON_EXCEPTION` 与下游调用使用入参 isolate。

  * `vtable_.xxx_ = &Virtual_Xxx` 绑定保留，按新签名自动对齐。

* 目的：完成具体类型实现层的 ABI 收敛。

### 5) 上层调用点同步改造（无过渡）

* 文件：`src/interpreter/interpreter-dispatcher.cc`

* 变更：

  * `CompareOp` 与 `ContainsOp` 调用统一传 `isolate_`。

* 文件：其余调用方（按检索结果逐一落地）

  * `src/objects/*`、`test/unittests/*`、`test/unittests/test-utils.cc` 等涉及 `PyObject::Greater/Less/.../Contains` 的调用点。

  * 规则：有 `isolate/isolate_` 直接透传；否则使用 `Isolate::Current()`。

* 目的：保证全仓在新接口下编译闭环。

### 6) 单元测试调用点批量更新

* 文件范围：

  * `test/unittests/objects/py-smi-unittest.cc`

  * `test/unittests/objects/py-float-unittest.cc`

  * `test/unittests/objects/py-list-unittest.cc`

  * `test/unittests/objects/py-string-unittest.cc`

  * `test/unittests/objects/py-dict-unittest.cc`

  * `test/unittests/objects/py-tuple-unittest.cc`

  * `test/unittests/heap/gc-unittest.cc`

  * `test/unittests/test-utils.cc`

* 变更：

  * 比较与 contains 相关调用改为显式 isolate 入参。

  * 测试内优先使用 `isolate_`（符合本期约定），必要时使用 `Isolate::Current()`。

* 目的：与主代码接口一致，避免编译断裂。

## Assumptions & Decisions

* 决策已锁定：

  * 本期采用“一步到位”改造，不使用过渡 API。

  * 允许在 `klass-vtable.h` 引入比较专用临时签名类型。

  * 调用点传参策略：优先 `isolate/isolate_`，否则 `Isolate::Current()`。

  * 单测调用点可直接使用 `isolate_`。

* 默认保持：

  * 比较/contains 语义、错误类型与真值转换逻辑不变，仅更改调用约定。

  * 不扩展本期范围之外功能。

## Verification

* 编译验证：

  * `ninja -C out/debug ut`

* 回归验证（比较/contains 重点）：

  * `out/debug/ut.exe --gtest_filter="*PySmi*:*PyFloat*:*PyString*:*PyList*:*PyTuple*:*PyDict*:*PyObject*"`

  * `out/debug/ut.exe --gtest_filter="*Interpreter*Ops*:*interpreter*ops*:*Interpreter*Dict*:*interpreter*dict*"`

  * `out/debug/ut.exe --gtest_filter="*GcTest*"`

* 通过标准：

  * 编译通过、关键回归通过、无新增 diagnostics。

  * 全仓不再存在旧签名的比较/contains `PyObject` 调用（通过检索确认）。

