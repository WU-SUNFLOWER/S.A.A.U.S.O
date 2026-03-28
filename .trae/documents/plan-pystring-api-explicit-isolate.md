# PyString API 收口并对齐容器风格计划

## Summary

* 目标：将 `PyString` 的对象层 API 彻底收口到与 `PyList / PyDict / PyTuple` 一致的显式 `Isolate*` 风格，并清理本轮范围内剩余的桥接点。

* 当前仓库已完成主干改造：`PyString::New / Clone / Append` 已是显式 `Isolate*` 版本，`NewInstance` 已退场，`StringTable` 初始化也已切到显式 isolate。

* 因此本计划不再是“从零开始改 API”，而是基于当前落地状态做收尾：补齐 `py-string.cc` 中残余的 4 个直接 `Isolate::Current()` 点，收紧少量 helper 调用面，并完成文档与验证闭环。

* 本轮仍不触碰 `HandleScope` 基建治理，不扩展到 `ST / ST_TAGGED` 宏访问模型，也不处理字符串表示分层。

## Current State Analysis

### 1. 已落地的 API 收口基线

* `PyString` 头文件已经完成对象层 API 统一：

  * `New(Isolate* isolate, ...)`

  * `Clone(Isolate* isolate, ...)`

  * `Append(..., Isolate* isolate)`

  * 见 [py-string.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.h#L24-L43) 与 [py-string.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.h#L92-L99)

* `py-string.cc` 中对象工厂与拼接入口也已使用显式 `isolate->factory()`：

  * [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L37-L67)

  * [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L217-L221)

* `StringTable` 初始化已适配显式 isolate：

  * [string-table.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.cc#L12-L29)

### 2. 当前仍残留的直接 `Isolate::Current()` 点

* `py-string.cc` 里仍有 4 处直接桥接：

  * `FromInt` [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L77-L84)

  * `FromDouble` [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L92-L99)

  * `FromStdString` [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L102-L105)

  * `Slice` [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L203-L214)

* 它们不再阻塞对象层主 API 风格对齐，但会让 `PyString` 成为“表面一致、内部仍有局部桥接”的半收口状态。

### 3. 围绕 PyString 的剩余外侧桥接点

* `ModuleUtils::NewPyString` 仍直接取当前 isolate：

  * [module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.cc#L16-L22)

* builtin 模块注册宏上下文仍通过 `Isolate::Current()` 构造字符串：

  * [builtin-module-registry.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-registry.cc#L23-L29)

* 一些 repr / 文本 helper 仍通过 `PyString::FromStdString(...)` 间接回到当前 isolate 语义：

  * [runtime-py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-string.cc#L256-L260)

  * [runtime-py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-function.cc#L22-L49)

### 4. 本轮明确不处理的链路

* `HandleScope / EscapableHandleScope` 的显式 isolate 化仍属更重的基础设施治理，不在本轮范围。

* `ST / ST_TAGGED` 宏仍依赖 `Isolate::Current()->string_table()`，本轮不改其访问模型。

* `buffer()/writable_buffer()` 与未来 `cons/slice string` 表示分层是独立后续议题，不和本轮混做。

## Proposed Plan

### P1：完成 `py-string.cc` 内部剩余桥接点收口

* 修改文件：

  * `src/objects/py-string.h`

  * `src/objects/py-string.cc`

* 变更内容：

  * 为 `FromInt / FromDouble / FromStdString / Slice` 增补显式 `Isolate*` 版本，或调整其现有签名以消除内部直接 `Isolate::Current()`。

  * 统一改为用传入的 `isolate` 调用 `PyString::New(...)` 或 `factory()`，使 `py-string.cc` 不再保留对象层分配桥接。

* 设计要求：

  * 不引入双轨兼容壳；优先一次性改签名并同步调用方。

  * 不把 `EscapableHandleScope` 本身改造成显式 isolate 设施；如需要，保留其现有使用方式，仅消除 `PyString` 内部直接取 `Current()`。

### P2：同步收紧已有 isolate 的直接调用链

* 重点文件：

  * `src/runtime/runtime-py-string.cc`

  * `src/runtime/runtime-py-function.cc`

  * `src/runtime/runtime-exceptions.cc`

  * `src/objects/py-string-klass.cc`

  * `src/objects/py-base-exception-klass.cc`

  * `src/modules/module-name-resolver.cc`

  * `src/modules/module-utils.h`

  * `src/modules/module-utils.cc`

  * `src/modules/builtin-module-registry.cc`

* 变更内容：

  * 所有已经持有 `isolate / isolate_` 的调用点改为显式透传，不再借道 `PyString::FromStdString(...)` 或局部 `Current()`。

  * 对 `ModuleUtils::NewPyString` 这类 helper，优先改成接收 `Isolate*`，让模块链路与容器风格对齐。

  * 对 builtin 模块注册链路，优先评估是否可在 bootstrap 上下文显式传 isolate；若该点涉及更大初始化 ABI，则维持为明确记录的“本轮保留点”。

### P3：整理边界并更新计划文档

* 修改文件：

  * `.trae/documents/plan-pystring-api-explicit-isolate.md`

* 变更内容：

  * 回写真实落地状态，删除已过时的 `NewInstance` 叙述。

  * 明确区分“已完成基线”“本轮收尾项”“后续独立议题”。

* 目标：

  * 保证后续继续推进时，计划文件与仓库状态一致，避免重复讨论已完成部分。

### P4：验证与回归

* 静态验证：

  * 检索 `PyString::NewInstance\\(` 是否全仓清零。

  * 检索 `src/objects/py-string.cc` 是否仍残留 `Isolate::Current()`。

  * 用 diagnostics 检查新签名造成的漏传 isolate。

* 构建验证：

  * 构建 `ut`

* 定向回归：

  * 优先回归 `PyString` 对象层测试

  * 回归 `interpreter-str`、`native-print`、异常与 import 相关测试

  * 必要时补跑 handle / global-handle 相关测试，确认显式 API 与现有句柄语义兼容

## Assumptions & Decisions

* 假设：当前优先级是“把 PyString API 真正收住并对齐容器风格”，不是全面推进 `HandleScope` 治理。

* 决策：沿用“一步到位改签名、同步改调用方”的策略，不保留旧接口兼容层。

* 决策：若某些初始化宏上下文暂时无法优雅显式透传 isolate，则把它们视作独立后续议题，不为了本轮目标引入复杂过渡设计。

* 决策：不处理 `buffer()/writable_buffer()` 公开形态，不处理字符串内部表示优化，不将任务扩大为 `cons/slice string` 架构演进。

## Completion Criteria

* `PyString` 对象层 API 全部与容器风格一致，调用入口不再混有旧式桥接语义。

* `src/objects/py-string.cc` 内不再直接出现 `Isolate::Current()`。

* 直接调用链完成同步，无新增 diagnostics，相关构建与定向回归通过。

* 计划文档与实际代码状态一致，可作为下一步继续推进 `PyString` 剩余治理的可靠基线。
