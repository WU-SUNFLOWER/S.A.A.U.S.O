# 内建模块架构治理与统一注册（Plan）

## 1. Summary

本次治理聚焦 `src/modules`（不扩展到 importer/loader 语义重构），目标是在**保持现有行为优先**的前提下，完成内建模块实现的统一化与去重，降低 ABI 变更和后续扩展成本。\
核心方向：

* 统一模块内 native 函数 ABI 声明方式（复用 builtins ABI 宏定义，不再手写重复签名）。

* 抽取并复用内建模块通用安装 helper（模块函数安装、无 kwargs 报错）。

* 用 X-macro 收敛各模块“逐条注册”样板。

* 修复 code review 发现的模块内明显缺陷，并补齐回归测试。

## 2. Current State Analysis

### 2.1 你指出的问题已确认存在

1. 重复代码明显：

* `FailNoKeywordArgs` 在 math/random/time 三处重复实现。

* `InstallFunc` 在 math/random/time 三处重复实现。\
  相关文件：

* `src/modules/builtin-math-module.cc`

* `src/modules/builtin-random-module.cc`

* `src/modules/builtin-time-module.cc`

1. 模块函数 ABI 与 builtins 宏割裂：

* `src/builtins/builtins-utils.h` 已提供 `BUILTIN(name)` 统一签名宏。

* 但 `src/modules` 中函数仍大量手写 `MaybeHandle<PyObject>(Isolate*, Handle<PyObject>, Handle<PyTuple>, Handle<PyDict>)`。

1. 模块函数注册样板冗长：

* `InitMathModule/InitRandomModule/InitTimeModule` 中均是手工 `InstallFunc(...)` 逐条注册，扩展成本高、易漏改。

### 2.2 额外发现的可维护性问题（本次同轮治理）

* `Math_Pow` 参数提取存在明显 bug：第二个参数提取结果写入了 `x` 而非 `y`，导致语义错误。\
  文件：`src/modules/builtin-math-module.cc`

* `random` 模块部分错误文案前缀不一致（如 `choice()/shuffle()` 未带 `random.` 前缀），影响一致性与测试可读性。\
  文件：`src/modules/builtin-random-module.cc`

* 模块级测试目前偏 happy-path，缺少对上述结构治理点和负路径的约束性断言。\
  文件：`test/unittests/interpreter/interpreter-{math,random,time}-module-unittest.cc`

## 3. Proposed Changes

### 3.1 新增模块通用 helper 与宏（治理基座）

#### 3.1.1 新增文件：`src/modules/builtin-module-utils.h`

新增并对外声明以下能力（附中文注释）：

* `using`/宏：为模块函数提供与 `BUILTIN(name)`一致的 ABI 声明入口（避免 modules 侧继续手写签名）。

* `Maybe<void> InstallBuiltinModuleFunc(...)`：统一“NativeFuncPointer -> PyFunction -> module dict”安装流程。

* `Maybe<void> InstallBuiltinModuleFuncsFromSpec(...)`：支持按表（name + func）批量注册。

* `void ThrowNoKeywordArgsError(const char* module_name, const char* func_name)`：统一 kwargs 拒绝报错格式。

* `struct BuiltinModuleFuncSpec { const char* name; NativeFuncPointer func; };`

#### 3.1.2 新增文件：`src/modules/builtin-module-utils.cc`

实现上述 helper，复用现有 `FunctionTemplateInfo` 与 `Factory::NewPyFunctionWithTemplate` 逻辑。\
要求：

* 严格遵守 `Maybe/RETURN_ON_EXCEPTION` 传播契约。

* 注释说明“为什么抽象到 modules 层而不是复用 builtins-utils（避免 access\_flag/owner\_type 语义耦合）”。

#### 3.1.3 修改文件：`BUILD.gn`

将 `src/modules/builtin-module-utils.{h,cc}` 纳入 `saauso_core` 源集。

***

### 3.2 改造三大内建模块以消除重复和样板

#### 3.2.1 修改文件：`src/modules/builtin-math-module.cc`

* 删除本地重复 `InstallFunc`、`FailNoKeywordArgs`。

* 引入 `builtin-module-utils.h`，改用统一 helper。

* 将模块函数声明切换到统一 ABI 宏。

* 引入 `MATH_MODULE_FUNC_LIST(V)` X-macro，并在 `InitMathModule` 中通过批量注册 helper 一次性安装。

* 顺手修复 `Math_Pow` 的参数提取 bug（第二参写入 `y`）。

* 保持 `pi/e/tau/inf/nan` 常量初始化语义不变。

#### 3.2.2 修改文件：`src/modules/builtin-random-module.cc`

* 删除本地重复 `InstallFunc`、`FailNoKeywordArgs`。

* 改用统一 ABI 宏与 helper。

* 引入 `RANDOM_MODULE_FUNC_LIST(V)` 统一声明与注册。

* 统一少量错误文案前缀（仅做明显一致性修正，尽量不扩大行为变更面）。

#### 3.2.3 修改文件：`src/modules/builtin-time-module.cc`

* 删除本地重复 `InstallFunc`、`FailNoKeywordArgs`。

* 改用统一 ABI 宏与 helper。

* 引入 `TIME_MODULE_FUNC_LIST(V)` 统一声明与注册。

***

### 3.3 补齐回归测试（结构治理 + 缺陷修复）

#### 3.3.1 修改文件：`test/unittests/interpreter/interpreter-math-module-unittest.cc`

新增/补充用例：

* `math.pow(2, 3)` 返回 `8.0`，覆盖 `Math_Pow` 修复。

* 至少一条 kwargs 拒绝断言（例如 `math.sqrt(x=9)` 抛 `TypeError` 且文案包含 `takes no keyword arguments`）。

#### 3.3.2 修改文件：`test/unittests/interpreter/interpreter-random-module-unittest.cc`

新增/补充用例：

* kwargs 拒绝路径断言（如 `random.seed(a=1)`）。

* 保持已有随机相关 happy-path 不变，避免引入过度语义改动。

#### 3.3.3 修改文件：`test/unittests/interpreter/interpreter-time-module-unittest.cc`

新增/补充用例：

* kwargs 拒绝路径断言（如 `time.sleep(seconds=0.01)`）。

> 若新增独立测试文件，则同步更新 `test/unittests/BUILD.gn`；若仅在原文件追加用例则无需修改构建清单。

## 4. Assumptions & Decisions

* 决策：本轮只做 `src/modules` 侧治理，不动 `module-loader/module-importer` 的缓存时机与循环导入语义。

* 决策：错误策略选择“保持现有行为优先”；只修复明显错误和明显不一致文案，不做大规模 CPython 文案对齐。

* 假设：`src/builtins/builtins-utils.h` 的 ABI 宏属于通用约定，可在 modules 侧复用（通过 include），且不会引入语义歧义。

* 决策：模块函数注册使用“每模块本地 X-macro + 通用批量安装 helper”，避免引入跨模块超大总表，保持局部可读性。

## 5. Verification Steps

1. 构建验证（至少一次 Debug 或当前主开发配置）：

* `gn gen ...`

* `ninja -C ... ut`

1. 定向测试回归：

* 运行 `interpreter-math-module-unittest`

* 运行 `interpreter-random-module-unittest`

* 运行 `interpreter-time-module-unittest`

1. 结构治理验收（静态检查）：

* 确认 `builtin-{math,random,time}-module.cc` 不再定义本地 `InstallFunc`/`FailNoKeywordArgs`。

* 确认三模块函数声明已切换到统一 ABI 宏。

* 确认三模块 `Init*Module` 使用 X-macro/批量注册路径，不再逐条手写安装。

1. 行为验收：

* `math.pow(2, 3)` 返回值正确。

* kwargs 负路径用例稳定通过。

* 原有 happy-path 用例全部通过。

