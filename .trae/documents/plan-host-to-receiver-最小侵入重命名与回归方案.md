# 计划：`host` → `receiver` 最小侵入重命名与回归测试

## Summary

* 目标：在不改变语义与行为的前提下，将调用链中语义不直观的 `host` 统一重命名为 `receiver`，并同步宏与关键注释术语。

* 约束：最小侵入、零功能改动、零协议改动（不改参数个数/调用约定），只做命名层治理。

* 回归策略：采用“扩展到解释器子集”，覆盖 CALL 主链路、方法绑定路径、native 调用路径、近期新增 `__init__`/`__call__` 用例。

## Current State Analysis

* `host` 是当前 VM 调用协议中的“隐式绑定接收者”槽位，贯穿：

  * `Execution::Call` 门面；

  * `Interpreter::CallPython/InvokeCallable/PopCallTarget`；

  * `FrameObjectBuilder` 首参数注入；

  * `PyObject::Call` 与各类 `Virtual_Call`/trampoline；

  * `NativeFuncPointer` 与 builtins 宏。

* 现状问题不是功能错误，而是可读性：

  * `host` 容易被误读为“宿主对象”而非“绑定接收者”；

  * 在 trampoline/type/native 路径语义可读性偏弱，增加维护心智负担。

* 当前已有相关回归基础：

  * `interpreter-call-stack-unittest.cc`

  * `interpreter-functions-unittest.cc`

  * `interpreter-custom-class-unittest.cc`

## Assumptions & Decisions

* 命名决策：统一使用 `receiver`。

* 改动范围决策：包含“函数声明签名参数名 + 宏与注释术语”，但不改 ABI/行为。

* 回归决策：在定向用例基础上，扩展到解释器子集执行。

* 非目标：

  * 不引入 `CallContext` 结构体；

  * 不调整 `self/receiver` 的参数顺序与数量；

  * 不做 descriptor 语义增强。

## Proposed Changes

### 1) 调用门面与解释器主链路重命名（仅命名）

* 文件：

  * `src/execution/execution.h`

  * `src/execution/execution.cc`

  * `src/interpreter/interpreter.h`

  * `src/interpreter/interpreter.cc`

  * `src/interpreter/interpreter-dispatcher.cc`

  * `src/interpreter/frame-object-builder.h`

  * `src/interpreter/frame-object-builder.cc`

* 做法：

  * 将参数/局部变量 `host` 统一重命名为 `receiver`；

  * `PopCallTarget` 等 helper 内部命名同步；

  * 与绑定首参数注入相关注释术语同步为“receiver/绑定接收者”。

* 原因：

  * 保留原协议，降低认知歧义，提升跨文件可读性。

### 2) 对象调用接口链重命名（仅命名）

* 文件：

  * `src/objects/py-object.h`

  * `src/objects/py-object.cc`

  * `src/objects/klass-vtable-trampolines.h`

  * `src/objects/klass-vtable-trampolines.cc`

  * `src/objects/py-function-klass.h`

  * `src/objects/py-function-klass.cc`

  * `src/objects/py-type-object-klass.h`

  * `src/objects/py-type-object-klass.cc`

  * `src/objects/py-object-klass.h`

  * `src/objects/py-object-klass.cc`

* 做法：

  * `Virtual_Call` 及相关调用点参数名统一 `receiver`；

  * trampoline 的 `Execution::Call(..., receiver, ...)` 语义注释同步；

  * 不改任何调用逻辑与分支行为。

* 原因：

  * 确保接口层命名一致，避免“上层 receiver / 下层 host”混杂。

### 3) Native 函数签名与宏术语同步（仅命名）

* 文件：

  * `src/objects/native-function-helpers.h`

  * `src/builtins/builtins-utils.h`

  * `src/modules/builtin-math-module.cc`

  * `src/modules/builtin-random-module.cc`

  * `src/modules/builtin-time-module.cc`

  * （若受影响）`test/unittests/test-helpers.h`

  * （若受影响）`test/unittests/test-helpers.cc`

* 做法：

  * `NativeFuncPointer` 形参名 `host` 改为 `receiver`；

  * `BUILTIN_METHOD` 宏与对应实现参数命名同步；

  * 关键注释术语同步。

* 原因：

  * 统一“可调用对象 ABI 入口”的术语，避免 builtins 与 VM 主链路语义断层。

### 4) 回归测试方案（不新增行为，只做稳定性验证）

* 定向验证（必须）：

  * `BasicInterpreterTest.ListSubclassInitCanInvokeBaseListInit`

  * `BasicInterpreterTest.InitSlotBridgeStaysCallableAfterAttributeLoad`

  * `BasicInterpreterTest.DictInitSlotBridgeStaysCallableAfterAttributeLoad`

  * `BasicInterpreterTest.ObjectInitRejectsArgsWhenTypeDefinesNeitherInitNorNew`

  * `BasicInterpreterTest.ObjectInitRejectsArgsWhenTypeOverridesInit`

  * `BasicInterpreterTest.DiamondMroCallDispatchUsesNearestOverride`

* 解释器子集扩展（按用户选择）：

  * `interpreter-call-stack-unittest.cc` 对应用例集合

  * `interpreter-functions-unittest.cc` 与方法绑定/参数传递相关用例集合

  * `interpreter-custom-class-unittest.cc` 相关调用/MRO 用例集合

* 目标：

  * 证明“仅命名改动”未影响调用路径与异常行为。

## Verification Steps

1. 构建：

   * `gn gen out/unittest --args="is_asan=true"`（若目录已存在则跳过）

   * `ninja -C out/unittest ut`
2. 运行定向 gtest 过滤器，确保上述 6 个关键用例全部通过。
3. 运行解释器子集回归（按文件或过滤器批次），确保通过。
4. 获取诊断（IDE diagnostics）确认无新增编译/静态错误。
5. 最终核验：

   * 代码中不再出现调用协议语义上的 `host` 命名残留（允许第三方/无关上下文例外）。

