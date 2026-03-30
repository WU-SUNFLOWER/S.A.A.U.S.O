# ST 阶段二：调用点透传实施计划

## Summary

* 目标：在已完成 [string-table.h](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h#L20-L21) 宏签名改造的前提下，推进阶段二“调用点透传”，将仓内所有 `ST(...)` / `ST_TAGGED(...)` 调用统一迁移到显式 `Isolate*` 形态。

* 范围：仅处理 string-table 宏调用点与其直接依赖的少量 helper 签名透传；不在本轮治理 `HandleScope` / `CreateHandle` / `handle(...)` 内核。

* 交付标准：源码中不再存在旧签名 `ST(x)` / `ST_TAGGED(x)` 调用；所有调用点都明确传入 `isolate` 或 `isolate_`；相关单测与构建通过。

## Current State Analysis

### 1. 宏接口现状

* 当前宏定义已切换为双参形式，见 [string-table.h:L20-L21](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h#L20-L21)：

  * `ST_TAGGED(x, isolate)`

  * `ST(x, isolate)`

* `ST(x, isolate)` 仍会通过 `handle(...)` 走本地句柄创建链，因此本轮仅实现“调用面显式 isolate 化”，不宣称底层已彻底去 `Current()`。

### 2. 调用点盘点

* 基于仓内检索，真实宏调用点共 86 处。

* 已适配调用点仅有 3 处，均为 `ST_TAGGED(..., isolate)`：

  * [accessor-proxy.cc:L47-L57](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/accessor-proxy.cc#L47-L57)

  * [interpreter-dispatcher.cc:L358-L364](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc#L358-L364)

* 绝大多数 `ST(...)` 仍保持旧调用方式，需要分批迁移。

### 3. 迁移难度分层

* **A 类：可机械替换，作用域内已有** **`isolate_`** **成员**

  * [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc)

  * [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc)

  * [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc)

  * [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)

  * [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)

  * [module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)

* **B 类：可机械替换，作用域内已有** **`isolate`** **形参**

  * [builtins-io.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-io.cc)

  * [klass-vtable-trampolines.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable-trampolines.cc)

  * [builtin-module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-utils.cc)

  * [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)

  * [py-oddballs-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-oddballs-klass.cc)

  * [py-base-exception-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-base-exception-klass.cc)

  * [module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.cc)

  * [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)

  * [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)

  * [runtime-intrinsics.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-intrinsics.cc)

  * [runtime-exec.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exec.cc)

  * [klass-vtable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc)

* **C 类：需要先补 helper 签名透传**

  * [runtime-exceptions.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc)

  * 当前 [GetExceptionStringHandle](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc#L58-L71) 仅接收 `ExceptionType`，但内部调用 `ST(...)`，是本轮唯一已确认的结构性阻塞点。

### 4. 高风险与高密度点

* [klass-vtable-trampolines.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable-trampolines.cc) 是 `ST(...)` 最密集文件，约 26 处，且涉及大量魔法方法 trampoline，建议单独作为一个审阅批次。

* [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc) 与 [klass-vtable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc) 含宏展开点，改动虽小但影响面广，需要重点检查展开后的调用语义。

## Proposed Changes

### 第 1 步：修正唯一的签名传播阻塞点

* 修改 [runtime-exceptions.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc)：

  * 将 `GetExceptionStringHandle(ExceptionType type)` 改为 `GetExceptionStringHandle(Isolate* isolate, ExceptionType type)`。

  * 将内部 `ST(string_table_name)` 与 `ST(runtime_err)` 改为显式 `ST(..., isolate)`。

  * 同步更新 [runtime-exceptions.cc:L145](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc#L145) 的调用点。

* 原因：先清除唯一已确认的 helper 层阻塞，再进入大批量机械替换，避免“改宏时才发现需要补签名”的打断式返工。

### 第 2 步：迁移 `isolate_` 成员持有型文件

* 批量修改以下文件中所有旧 `ST(...)` 为 `ST(..., isolate_)`：

  * [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc)

  * [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc)

  * [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc)

  * [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)

  * [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)

  * [module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)

* 执行方式：

  * 逐文件替换，不引入兼容宏。

  * 若同一函数内同时存在 `ST_TAGGED`，统一保持参数名风格一致。

  * 宏展开体一并调整，例如 [builtin-bootstrapper.cc:L130-L132](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc#L130-L132)。

* 原因：该批文件已天然持有成员 `isolate_`，是最稳妥、最接近纯机械替换的一组。

### 第 3 步：迁移 `isolate` 形参持有型文件

* 批量修改以下文件中所有旧 `ST(...)` 为 `ST(..., isolate)`：

  * [builtins-io.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-io.cc)

  * [klass-vtable-trampolines.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable-trampolines.cc)

  * [builtin-module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-utils.cc)

  * [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)

  * [py-oddballs-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-oddballs-klass.cc)

  * [py-base-exception-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-base-exception-klass.cc)

  * [module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.cc)

  * [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)

  * [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)

  * [runtime-intrinsics.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-intrinsics.cc)

  * [runtime-exec.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exec.cc)

  * [klass-vtable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc)

* 执行方式：

  * 普通函数直接替换为 `ST(..., isolate)`。

  * 宏体内同步替换，例如 [klass-vtable.cc:L75-L86](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc#L75-L86)。

  * [klass-vtable-trampolines.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable-trampolines.cc) 单独检查每个 trampoline 已持有的 isolate 变量名，避免局部形参名不一致。

* 原因：这批文件虽然数量更多，但 isolate 来源同样清晰，适合第二波机械推进。

### 第 4 步：清点并收尾 `ST_TAGGED`

* 复查 [accessor-proxy.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/accessor-proxy.cc) 与 [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc) 已迁移的 `ST_TAGGED` 调用，确认风格与本轮 `ST` 替换一致。

* 对全仓进行检索，确认不存在旧 `ST_TAGGED(x)` / `ST(x)` 一参调用残留。

* 原因：本轮目标是“调用点透传”闭环，而不是只完成大部分替换。

## Assumptions & Decisions

* 决策 1：本轮只做“接口层显式 isolate 化”，不在本轮触碰 `HandleScope` / `CreateHandle` / `handle(...)` 内核。

* 决策 2：不引入兼容宏或临时重载；旧签名直接清零，避免双轨期拖长。

* 决策 3：显式 isolate 参数名遵循现有局部命名，不做统一重命名：

  * 成员场景用 `isolate_`

  * 自由函数/方法形参场景用 `isolate`

* 决策 4：若实施过程中发现新增“无 isolate 却调用 `ST(...)`”的 helper，采用与 [runtime-exceptions.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc) 相同策略：先补 helper 签名，再继续替换；不退回引入临时 `Current()`。

* 假设 1：用户已完成 [string-table.h:L20-L21](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h#L20-L21) 的宏签名改造，且当前工作树允许继续在此基础上修改调用点。

## Verification

* 静态检查：

  * 全仓检索 `ST(` 与 `ST_TAGGED(`，确认不再存在旧一参调用。

  * 检查 `runtime-exceptions.cc` 的 helper 签名与调用点是否一致。

* 构建验证：

  * 参考 [AGENTS.md:L644-L647](file:///e:/MyProject/S.A.A.U.S.O/AGENTS.md#L644-L647) 生成并构建 debug 产物，至少确保核心源码可编译。

* 回归验证：

  * 参考 [AGENTS.md:L663-L664](file:///e:/MyProject/S.A.A.U.S.O/AGENTS.md#L663-L664) 运行 `ut` 统一测试入口。

  * 若需要缩小验证范围，优先关注受影响模块对应单测：

    * `test/unittests/objects/`

    * `test/unittests/interpreter/`

    * `test/unittests/handles/`

* 通过标准：

  * 编译通过

  * `ut` 通过

  * 无旧签名宏残留

  * 未新增 `Isolate::Current()` 兜底调用

