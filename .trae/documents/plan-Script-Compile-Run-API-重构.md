# Script::Compile / Script::Run API 改造计划

## Summary

- 目标：把公开 Embedder API 中的 `Script::Compile / Script::Run` 从“源码字符串套壳 + Run 时现编译”改造为“`Compile` 阶段真实编译为模块级 `PyFunction`，`Run` 阶段直接调用统一执行链路”。
- 范围：优先完成 API 实现与回归测试，不引入新的 helper 分层；同时遵循当前 MVP 决策：
  - `Script::Run(context)` 使用 `globals = Context`、`locals = 新建空 i::PyDict`。
  - 不向 `globals` 自动注入 `__builtins__`。
  - 暂不自动注入 `__name__`，但在源码中补 `TODO` 明确未来需要统一规则。
- 验收：完成代码修改后，执行一轮 `embedder_ut` 与 `./samples` 全体 demo 的全量回归。

## Current State Analysis

- 公开 API 声明位于 [saauso-script.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-script.h#L16-L23)，接口名已经表达为“Compile/Run”两阶段。
- 当前实现中，[api-script.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-script.cc#L17-L35) 的 `Script::Compile` 在开启 CPython 前端时仅把源码保存成 `PyString` 并返回，不是真实编译。
- 当前 [api-script.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-script.cc#L38-L72) 的 `Script::Run` 会重新把脚本文本交给 `Runtime_ExecutePythonSourceCode(...)`，因此每次 `Run` 都重新编译。
- VM 内部主链路已经是：
  - [compiler.cc](file:///e:/MyProject/S.A.A.U.S.O/src/code/compiler.cc#L27-L67) `Compiler::CompileSource(...)` 产出模块级 boilerplate `PyFunction`
  - [execution.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/execution.cc#L20-L35) `Execution::CallScript(...)` 负责绑定 `func_globals` 并执行
- 当前 `Script::Run` 的环境绑定是“`locals = Context`，`globals = 新建空 dict`”，见 [api-script.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-script.cc#L58-L63)。本次按已确认的 MVP 方案改为“`globals = Context`，`locals = 新建空 dict`”。
- 现有直接覆盖 `Script::Compile/Run` 的核心测试位于 [embedder-script-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/embedder/embedder-script-unittest.cc#L131-L324)。

## Assumptions & Decisions

- 底层承载对象首轮直接复用 `PyFunction`，不新增内部 `Script` 堆对象。
- `Compile` 失败语义保持 `MaybeLocal<Script>` 为空，并继续经 API 边界异常结算逻辑与 `TryCatch` 协同。
- `Run` 失败语义保持 `MaybeLocal<Value>` 为空，并继续走现有 API 边界异常结算逻辑。
- `Script::Run(context)` 的执行环境固定为：
  - `globals`：`Context` 底层的 `PyDict`
  - `locals`：每次运行时新建的空 `i::PyDict`
- 本次不新增“globals 初始化 helper”，也不在 `globals` 中自动镜像 `__builtins__`。
- 本次不自动注入 `__name__`，但会在实现中标注 `TODO`，提示未来明确注入职责归属。
- 公开 API 形态不变：不修改 [saauso-script.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-script.h#L16-L23) 的函数签名。

## Proposed Changes

### 1. `src/api/api-script.cc`

- 把 `Script::Compile` 改为：
  - 使用 `api::RequireExplicitIsolate(isolate)` 获取内部 isolate。
  - 将 `Local<String>` 转换为 `PyString` 源码对象。
  - 调用 [compiler.cc](file:///e:/MyProject/S.A.A.U.S.O/src/code/compiler.cc#L27-L67) 的 `Compiler::CompileSource(...)` 得到 `Handle<PyFunction>`。
  - 编译失败时执行 `FinalizePendingExceptionAtApiBoundary(i_isolate)` 并返回空 `MaybeLocal<Script>`。
  - 成功时将 `PyFunction` 直接包装为 `Local<Script>` 返回。
- 把 `Script::Run` 改为：
  - 校验 `this` 底层对象确实为 `PyFunction`，不再要求是 `PyString`。
  - 打开 `Context` 并取出其底层 `PyDict` 作为 `globals`。
  - 运行前创建一个新的空 `PyDict` 作为 `locals`。
  - 调用 [execution.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/execution.cc#L20-L35) 的 `Execution::CallScript(...)` 执行已编译脚本。
  - 执行失败时执行 `FinalizePendingExceptionAtApiBoundary(i_isolate)` 并返回空 `MaybeLocal<Value>`。
  - 在合适位置补一条不改变行为的 `TODO`：未来明确 `__name__` 注入规则归属。
- 为了支撑上述改造，按最小改动补齐所需 include，并保持 `src/api` 现有私有桥接依赖边界，不直接引入不必要的新层次。

### 2. `test/embedder/embedder-script-unittest.cc`

- 调整并补强与 `Script::Compile/Run` 相关的断言，确保覆盖以下行为：
  - `Compile` 在前端开启时返回“真实可运行脚本对象”，`Run` 只负责执行。
  - `Compile` 失败时由 `TryCatch` 在编译阶段直接捕获，而不是拖到 `Run`。
  - 同一 `Script` 可重复在不同 `Context` 上运行，且每次运行都使用“`globals = Context`、`locals = 空 dict`”的 MVP 语义。
  - 脚本顶层定义的函数后续读取全局变量时，应绑定到运行时提供的 `Context globals`。
- 尽量复用现有测试结构与命名风格，避免新建过多无关测试文件。

### 3. `samples/*.cc`（只做兼容性回归，原则上不改）

- [hello-world.cc](file:///e:/MyProject/S.A.A.U.S.O/samples/hello-world.cc)
- [game-engine-demo.cc](file:///e:/MyProject/S.A.A.U.S.O/samples/game-engine-demo.cc)
- [inject-cpp-func-to-python-world.cc](file:///e:/MyProject/S.A.A.U.S.O/samples/inject-cpp-func-to-python-world.cc)
- [call-python-func-in-cpp.cc](file:///e:/MyProject/S.A.A.U.S.O/samples/call-python-func-in-cpp.cc)
- [catch-python-exception.cc](file:///e:/MyProject/S.A.A.U.S.O/samples/catch-python-exception.cc)

- 这些示例的 API 用法不应因本次改造而变化，计划中仅做运行级回归。
- 若回归暴露出样例对旧“Run 时现编译”行为的隐性依赖，再按最小原则修补样例代码。

### 4. `include/saauso-script.h` / 文档文件（按需审计）

- [saauso-script.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-script.h#L16-L23) 的接口签名预计无需变更。
- [Saauso-Embedder-API-User-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Embedder-API-User-Guide.md) 当前文字本就把 `Compile` 描述为“编译为可执行对象”；若实现与文档重新一致，则无需额外改文档。
- 实施阶段会快速审计相关注释；若存在与新语义冲突的说明，再做最小同步修正。

## Implementation Steps

1. 先修改 `src/api/api-script.cc`，完成 `Script` 底层承载从 `PyString` 切到 `PyFunction`，并让 `Run` 走 `Execution::CallScript(...)`。
2. 重新审视异常路径，确保 `Compile` / `Run` 两阶段的 `FinalizePendingExceptionAtApiBoundary(...)` 时机正确。
3. 修改 `test/embedder/embedder-script-unittest.cc`，让单测断言与新语义一致，并补至少一条“函数读取 Context globals”的回归用例。
4. 编译并运行 `embedder_ut`。
5. 编译并运行 `./samples` 全体 demo，确认行为未回退。
6. 若回归中发现样例或注释依赖旧行为，再做最小范围收尾修正。

## Verification

- 构建并运行 `test/embedder:embedder_ut`，重点关注：
  - `Script::Compile` 失败时机是否前移且 `TryCatch` 行为正确
  - 同一 `Script` 多次 `Run` 是否稳定
  - 跨 `Context` 执行时全局变量解析是否符合本次 MVP 约定
- 构建并运行以下 sample target，做全量回归：
  - `embedder_hello_world`
  - `embedder_game_engine_demo`
  - `inject_cpp_func`
  - `call_python_func`
  - `catch_exception`
- 若测试结果显示 `samples` 中有脚本依赖 `__name__` 或旧 globals/locals 行为，则记录具体现象并以最小改动修复。

## Out of Scope

- 新增独立的 globals 初始化 helper。
- 系统性重构 `Runtime_Execute*` / `Execution::CallScript` 的职责边界。
- 调整解释器对 builtins 的查找源，使其从 `globals["__builtins__"]` 动态读取。
- 完整定义并实现 `__name__ / __package__ / __file__ / __path__ / __spec__` 的统一注入策略。
