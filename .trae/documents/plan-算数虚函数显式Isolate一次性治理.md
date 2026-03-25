# 算数虚函数显式 Isolate* 一次性治理计划（无过渡 API）

## Summary
- 目标：对 `sub/add/mul/div/floor_div/mod/hash` 统一改为显式传入 `Isolate*`，并且一次性完成调用约定升级，不引入中间过渡 API。
- 范围：`py-object.h/.cc` 对外入口、`klass-vtable` 槽位签名、`klass-vtable-trampolines`、各内建类型 `klass` 对应虚函数实现，以及所有直接调用点。
- 约束：不新增桥接重载、不保留旧签名双轨、不分阶段兼容；同一改造批次完成编译闭环与回归验证。

## Current State Analysis
- `KlassVtable` 当前将这 7 个槽位定义为无 `Isolate*` 签名：
  - `add/sub/mul/div/floor_div/mod` 使用 `VirtualFuncType_Maybe_1_2`
  - `hash` 使用 `VirtualFuncType_MaybeHash`
  - 位置：`src/objects/klass-vtable.h`
- `PyObject` 对外入口当前也为无 `Isolate*`：
  - `Add/Sub/Mul/Div/FloorDiv/Mod/Hash`
  - 位置：`src/objects/py-object.h`、`src/objects/py-object.cc`
- 上层调用点：
  - 二元运算分发：`src/interpreter/interpreter-dispatcher.cc`
  - hash 热路径：`src/objects/py-dict.cc`
- trampoline 当前在函数体内用 `Isolate::Current()` 调 runtime：
  - `src/objects/klass-vtable-trampolines.cc`
- 受影响内建 `klass`（声明+定义）：
  - `py-smi-klass.h/.cc`
  - `py-float-klass.h/.cc`
  - `py-string-klass.h/.cc`
  - `py-list-klass.h/.cc`
  - `py-oddballs-klass.h/.cc`
  - `py-type-object-klass.h/.cc`
- `Runtime_InvokeMagicOperationMethod` 已是显式 `Isolate*`，无需改接口：
  - `src/runtime/runtime-reflection.h/.cc`

## Proposed Changes

### 1) 升级 vtable 槽位签名（核心 ABI 变更）
- 文件：`src/objects/klass-vtable.h`
- 变更：
  - 为算数与 hash 槽位定义带 `Isolate*` 的函数指针类型（例如二元算术与单参 hash 各自独立类型）。
  - 在 `KLASS_VTABLE_SLOT_EXPOSED` 中将 `add/sub/mul/div/floor_div/mod/hash` 全部切到新签名类型。
- 目的：在类型系统层强制所有实现与调用链显式传递 `Isolate*`。

### 2) 升级 PyObject 对外入口并同步实现
- 文件：`src/objects/py-object.h`、`src/objects/py-object.cc`
- 变更：
  - 将 `Add/Sub/Mul/Div/FloorDiv/Mod/Hash` 统一改为首参 `Isolate* isolate`。
  - 函数体内调用 vtable 槽位时透传 `isolate`。
  - 保留现有 fast path 语义（如 smi-smi 的 add/sub/mul/floor_div/mod），仅改变调用约定。
- 目的：将显式 `Isolate*` 提升到公共调用边界，避免隐式上下文依赖。

### 3) 同步修改上层调用方（一次性）
- 文件：`src/interpreter/interpreter-dispatcher.cc`
- 变更：
  - `PyObject::{Add/Sub/Mul/Div/FloorDiv/Mod}` 调用统一改为传入当前解释器持有的 `isolate` 变量（不在调用点新建桥接 API）。
- 文件：`src/objects/py-dict.cc`
- 变更：
  - `PyObject::Hash(...)` 调用改为显式传入当前上下文 isolate（本文件已可用 `Isolate::Current()`）。
- 目的：打通上层到对象层的显式 isolate 传递。

### 4) 升级 trampoline 签名与实现
- 文件：`src/objects/klass-vtable-trampolines.h`、`src/objects/klass-vtable-trampolines.cc`
- 变更：
  - `Add/Sub/Mul/Div/FloorDiv/Mod/Hash` 静态函数签名增加 `Isolate* isolate`。
  - 体内不再自行取 `Isolate::Current()`，改为使用入参 `isolate` 透传给 `Runtime_InvokeMagicOperationMethod`。
- 目的：消除 trampoline 层的隐式 isolate 获取，保持链路一致性。

### 5) 升级各内建类型 klass 虚函数签名
- 文件：
  - `src/objects/py-smi-klass.h/.cc`
  - `src/objects/py-float-klass.h/.cc`
  - `src/objects/py-string-klass.h/.cc`
  - `src/objects/py-list-klass.h/.cc`
  - `src/objects/py-oddballs-klass.h/.cc`
  - `src/objects/py-type-object-klass.h/.cc`
- 变更：
  - 对应 `Virtual_Add/Sub/Mul/Div/FloorDiv/Mod/Hash` 签名统一增加 `Isolate* isolate`。
  - 函数体优先透传入参 isolate；若局部仍需 `ASSIGN_RETURN_ON_EXCEPTION`，使用入参 isolate。
  - vtable 绑定点（`vtable_.add_ = &Virtual_Add` 等）保持不变，仅类型对齐新签名。
- 目的：确保所有具体虚函数实现和槽位新 ABI 完全一致。

### 6) 全局编译错误清零与尾部收口
- 范围：全仓受签名升级影响的调用点与函数声明/定义不一致处。
- 处理原则：
  - 仅做签名升级与参数透传，不做语义扩展。
  - 不引入任何过渡 API 或临时桥接函数。

## Assumptions & Decisions
- 已确认决策：
  - 不引入中间过渡 API。
  - 允许在同次改造中让上层调用方显式传入 `Isolate::Current()` 或已有 `isolate` 变量。
- 本计划默认：
  - 行为语义保持不变（仅调用约定变化）。
  - `Runtime_InvokeMagicOperationMethod` 维持现有接口，不额外改动 runtime-reflection API。
  - 改造以“单次原子提交”为执行单位，避免中途不可编译状态长期存在。

## Verification
- 构建验证：
  - 编译目标：`ut`
  - 建议命令：`ninja -C out/debug ut`
- 回归验证（最小必跑集）：
  - `out/debug/ut --gtest_filter=*PySmi*:*PyFloat*:*PyString*:*PyList*:*PyDict*:*PyObject*`
  - `out/debug/ut --gtest_filter=*InterpreterOps*:*interpreter*ops*:*interpreter*dict*`
- 通过标准：
  - 全量编译通过（无签名不匹配错误）。
  - 以上用例通过且无新增崩溃/断言失败。
  - 算数与 hash 行为与改造前一致（重点看除零、类型错误、字典查找/rehash）。
