# Attr 显式 Isolate\* 三期一步到位治理计划

## Summary

* 目标：将 Attr 相关调用约定统一为显式传入 `Isolate*`，覆盖 `LookupAttr/GetAttr/GetAttrForCall/SetAttr`。

* 范围：`py-object.h/.cc` 对外入口、`klass-vtable` 对应槽位签名、`klass-vtable-trampolines`、各内建类型 klass 的 Attr 虚函数实现。

* 风格：参照第一期/第二期，单批次一步到位完成，不引入桥接重载或双轨过渡 API。

## Current State Analysis

* `klass-vtable` 中 Attr 槽位仍是无 isolate 签名：

  * `getattr`：`VirtualFuncType_MaybeGetAttr = Maybe<bool> (*)(OopHandle, OopHandle, bool, OopHandle&)`

  * `setattr`：`VirtualFuncType_Maybe_0_3 = MaybeHandle<PyObject> (*)(OopHandle, OopHandle, OopHandle)`

  * 文件：`src/objects/klass-vtable.h`

* `PyObject` Attr 对外接口仍无 isolate：

  * `LookupAttr/GetAttr/GetAttrForCall/SetAttr`

  * 文件：`src/objects/py-object.h`、`src/objects/py-object.cc`

* trampoline Attr 路径无 isolate 形参，并在实现中使用 `Isolate::Current()`：

  * `KlassVtableTrampolines::GetAttr/SetAttr`

  * 文件：`src/objects/klass-vtable-trampolines.h/.cc`

* klass 层 Attr 虚函数实现：

  * 通用实现：`PyObjectKlass::Generic_GetAttr/Generic_GetAttrForCall/Generic_SetAttr`

  * type 特化：`PyTypeObjectKlass::Virtual_GetAttr/Virtual_SetAttr`

  * 文件：`src/objects/py-object-klass.h/.cc`、`src/objects/py-type-object-klass.h/.cc`

* 主要调用点分布：

  * 解释器：`src/interpreter/interpreter-dispatcher.cc`

  * API：`src/api/api-objects.cc`

  * 单测：`test/unittests/objects/attribute-unittest.cc`、`py-module-unittest.cc`、`py-string-unittest.cc`

## Proposed Changes

### 1) 升级 vtable 的 Attr 专用签名（仅影响本期目标）

* 文件：`src/objects/klass-vtable.h`

* 变更：

  * 引入 Attr 专用临时签名类型：

    * `VirtualFuncType_MaybeGetAttrWithIsolate = Maybe<bool> (*)(Isolate*, OopHandle, OopHandle, bool, OopHandle&)`

    * `VirtualFuncType_MaybeSetAttrWithIsolate = MaybeHandle<PyObject> (*)(Isolate*, OopHandle, OopHandle, OopHandle)`

  * 将 `getattr/setattr` 槽位切换到新签名，其他槽位保持不变。

* 目的：在不污染无关虚函数 ABI 的前提下，完成 Attr 显式 isolate 契约。

### 2) 升级 PyObject Attr 对外接口与实现

* 文件：`src/objects/py-object.h`、`src/objects/py-object.cc`

* 变更：

  * 将以下接口统一改为首参 `Isolate* isolate`：

    * `LookupAttr/GetAttr/GetAttrForCall/SetAttr`

  * 实现中统一透传 isolate 到 vtable 与下游函数，移除 `Isolate::Current()` 依赖。

  * 保持现有语义：Lookup 三态、Get 抛错语义、GetAttrForCall 的 `self_or_null` 注入规则不变。

### 3) 升级 trampoline Attr 签名与透传逻辑

* 文件：`src/objects/klass-vtable-trampolines.h`、`src/objects/klass-vtable-trampolines.cc`

* 变更：

  * `GetAttr/SetAttr` 增加 `Isolate* isolate` 入参。

  * `SetAttr` 中 `Runtime_InvokeMagicOperationMethod` 改用入参 isolate，去除本地 `Isolate::Current()`。

  * `GetAttr` 透传 isolate 到 `PyObjectKlass::Generic_GetAttr`。

### 4) 升级 klass 层 Attr 虚函数实现

* 文件：`src/objects/py-object-klass.h/.cc`

* 变更：

  * `Generic_GetAttr/Generic_GetAttrForCall/Generic_SetAttr` 改为显式 isolate 入参。

  * 函数体内 `ASSIGN_RETURN_ON_EXCEPTION/RETURN_ON_EXCEPTION` 统一使用入参 isolate。

* 文件：`src/objects/py-type-object-klass.h/.cc`

* 变更：

  * `Virtual_GetAttr/Virtual_SetAttr` 增加 `Isolate* isolate` 入参。

  * 保持 Type MRO 查找与类属性写入语义不变，内部统一使用入参 isolate。

* 目的：确保 vtable 目标函数签名与实现完全一致，清除 Attr 链路隐式 isolate。

### 5) 全量调用点同步（一步到位）

* 文件：`src/interpreter/interpreter-dispatcher.cc`

  * `STORE_ATTR/LOAD_ATTR/LOAD_METHOD/import` 相关 Attr 调用改为传 `isolate_`。

* 文件：`src/api/api-objects.cc`

  * `PyObject::GetAttrForCall` 调用改为传 `internal_isolate`。

* 文件：`src/objects/py-object.cc`（内部调用）

  * `GetAttrForCall` fallback 调 `GetAttr`、`LookupAttr` 调 vtable、`SetAttr` 调 vtable 均透传 isolate。

* 规则：

  * 有有效 `isolate/isolate_` 直接透传；

  * 无现成上下文时使用 `Isolate::Current()`（按既有两期策略）。

### 6) 单测与测试工具同步

* 文件：

  * `test/unittests/objects/attribute-unittest.cc`

  * `test/unittests/objects/py-module-unittest.cc`

  * `test/unittests/objects/py-string-unittest.cc`（`GetAttr` 相关调用）

* 变更：

  * Attr 调用改为显式 isolate；测试内优先使用 `isolate_`。

## Assumptions & Decisions

* 决策：

  * 本期按一期/二期风格，一步到位完成，不引入过渡 API。

  * 允许在 `klass-vtable.h` 引入 Attr 专用临时签名以控制 ABI 影响面。

  * 调用点传参策略沿用前两期：优先 `isolate/isolate_`，否则 `Isolate::Current()`。

* 默认保持：

  * Attr 行为语义不变（包括异常类型、Lookup 三态、方法绑定语义）。

  * 不扩展到本期范围外的其它协议改造。

## Verification

* 编译验证：

  * `ninja -C out/debug ut`

* 重点回归：

  * `out/debug/ut.exe --gtest_filter="*attribute*:*PyModule*:*PyString*:*PyObject*"`

  * `out/debug/ut.exe --gtest_filter="*Interpreter*Ops*:*interpreter*ops*:*Import*"`

* 补充回归（防回归）：

  * `out/debug/ut.exe --gtest_filter="*PySmi*:*PyFloat*:*PyList*:*PyTuple*:*PyDict*"`

* 通过标准：

  * 编译通过、关键回归通过、无新增 diagnostics。

  * 全仓不再存在旧签名 Attr 入口调用（检索确认 `PyObject::LookupAttr/GetAttr/GetAttrForCall/SetAttr` 均带 isolate 参数）。

