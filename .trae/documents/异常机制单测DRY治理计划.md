# 异常机制单测 DRY 治理计划

## 背景与问题

随着 SAAUSO 的异常机制进入 MVP 可用状态，解释器端到端单测中出现了大量“期望抛异常”的断言样板代码，典型形态为：

* 触发执行（`Compiler::CompileSource` + `interpreter()->Run(...)` 或其它 API）

* `ASSERT_TRUE(isolate()->HasPendingException())`

* `Runtime_FormatPendingExceptionForStderr()` → 转 `std::string`

* `EXPECT_NE(msg.find("..."), std::string::npos)`（有时缺少 “got: msg” 的失败上下文）

* `isolate()->exception_state()->Clear()`（同一 TEST 内继续执行后续 sub-case 时必须清理）

重复热点集中在：

* [interpreter-builtins-seq-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-builtins-seq-unittest.cc)（同一 TEST 内多个 sub-case 反复复制粘贴）

* [interpreter-builtins-constructor-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-builtins-constructor-unittest.cc) 等其它解释器测试文件（多处同构用法）

* [interpreter-functions-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-functions-unittest.cc) 已出现“局部 lambda”抽象，但仅在单文件内生效

与此同时，`BasicInterpreterTest::RunScript(...)` 已经集中治理了“非预期异常逃逸测试”的格式化与清理逻辑（异常则 `ADD_FAILURE()` 并 `Clear()`），但它并不覆盖“期望异常”的测试，因此才产生了大量重复代码。

## 治理目标

* 系统性消除异常断言样板代码，避免复制粘贴导致的漏清理、失败信息不完整、字符串转换方式不一致等问题。

* 让测试用例聚焦在“触发条件/期望文案”，把“异常取出、格式化、清理”的细节下沉到统一抽象中，遵循 DRY/SOLID 与 Google C++ Style Guide 的可读性要求。

* 将治理范围优先覆盖 `test/unittests/interpreter/` 中所有 `Runtime_FormatPendingExceptionForStderr()` 的重复点（当前扫描到 7 个文件出现同构模式）。

## 非目标（本次不做）

* 不改动异常机制的运行时语义、错误文案本身或异常对象体系。

* 不把测试从 “substring contains” 改写为更强的结构化断言（例如精确类型/字段），除非现有测试已具备稳定 API。

* 不引入新的第三方测试依赖（例如 gMock 的 matcher），保持仓库当前 gtest 使用方式。

## 方案设计（抽象落点）

### 1) 在 `BasicInterpreterTest` 增加“期望异常”公共能力

在 [test-helpers.h](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/test-helpers.h) 的 `BasicInterpreterTest` 内新增两个静态 helper（作为解释器端到端测试的统一入口）：

* `static std::string TakePendingExceptionMessageForStderr();`

  * 断言存在 pending exception

  * 以 `Runtime_FormatPendingExceptionForStderr()` 格式化为字符串

  * 清理 pending exception（调用 `exception_state()->Clear()`）

  * 返回 `std::string` 供调用方做多次匹配或其它检查

* `static void RunScriptExpectExceptionContains(std::string_view source, std::string_view expected_substr, std::string_view file_name);`

  * 执行 `CompileSource + interpreter()->Run(...)`

  * 调用 `TakePendingExceptionMessageForStderr()` 获取并清理异常

  * 断言 `expected_substr` 为子串，并在失败时输出 “got: <msg>”

实现放在 [test-helpers.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/test-helpers.cc)，并确保 helper 自己创建 `HandleScope`，避免调用点遗漏 `HandleScope` 导致未来调整时出现脆弱依赖。

理由：

* 这些 helper 与 “解释器执行/异常格式化”强相关，放在 `BasicInterpreterTest` 可避免把 `test-utils` 变成“解释器专用工具集合”，保持职责边界清晰。

* 现有 `BasicInterpreterTest::RunScript` 已经承担类似的“统一异常处理”责任，此处扩展属于同一抽象层次。

### 2) 统一异常字符串转换方式

治理过程中统一使用 `PyString::ToStdString()`（仓库已有用法见 import 单测），避免在各测试文件里重复写 `buffer()+length()` 的样板代码。

## 代码改造范围（按文件落地）

对下列文件做“机械性替换”，把重复样板改为调用 `BasicInterpreterTest` 新增 helper（优先用 `RunScriptExpectExceptionContains`；需要复用消息时用 `TakePendingExceptionMessageForStderr`）：

* `test/unittests/interpreter/interpreter-builtins-seq-unittest.cc`

* `test/unittests/interpreter/interpreter-builtins-constructor-unittest.cc`

* `test/unittests/interpreter/interpreter-custom-class-unittest.cc`

* `test/unittests/interpreter/interpreter-closure-unittest.cc`

* `test/unittests/interpreter/interpreter-functions-unittest.cc`（删除局部 lambda，替换为公共 helper）

* `test/unittests/interpreter/interpreter-import-unittest.cc`

* 以及扫描到的其它 `Runtime_FormatPendingExceptionForStderr()` 使用点（以最终检索结果为准）

改造原则：

* 不改变每个 test 的语义结构（尤其是“一个 TEST 内多个 sub-case 串行执行”的写法），仅消除重复样板。

* 保持断言力度不下降，并在失败输出中统一带上完整异常字符串，提升可诊断性。

## 验证策略

* 构建并运行 `ut` 目标，确保所有单测通过。

* 额外做一次检索确认：`test/unittests/**` 中不再存在大段重复的“pending exception 格式化 + clear”样板；保留零星的特殊场景仅当确有必要（例如需要多次读取/比对异常字符串的细粒度断言）。

## 风险与回滚

* 风险：部分测试依赖“先 format 再 Clear”的细节顺序；helper 会保持该顺序，且只在取到字符串后再清理，避免语义变化。

* 风险：个别测试可能在同一段逻辑里需要继续保留 pending exception（例如后续 handler 逻辑依赖）；当前扫描未发现此类需求，若出现将为该测试保留局部处理并在 plan 复核阶段标注原因。

* 回滚：所有改动都是测试层抽象与调用点替换，出现问题可按文件级别回退到旧写法，不影响 runtime 代码。

