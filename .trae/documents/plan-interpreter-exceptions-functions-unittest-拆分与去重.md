# interpreter exceptions/functions 单测拆分与去重实施计划

## Summary

目标：对以下两个体积膨胀的解释器单测文件执行“按单一职责拆分 + 严格 100% 重复用例治理（仅删完全重复）”，并保证测试行为与覆盖语义不退化。

- `test/unittests/interpreter/interpreter-exceptions-unittest.cc`
- `test/unittests/interpreter/interpreter-functions-unittest.cc`

用户已确认的关键偏好：

- `exceptions` 采用 **4 文件细分**
- `functions` 采用 **5 文件细分**
- 去重采用 **严格仅删 100% 重复**

## Current State Analysis

### 1) 构建入口现状

`test/unittests/BUILD.gn` 当前直接编译上述两个大文件（`sources += [...]` 中包含）：

- `./interpreter/interpreter-exceptions-unittest.cc`
- `./interpreter/interpreter-functions-unittest.cc`

拆分后必须同步更新该文件，替换为新文件清单，否则会出现重复编译或漏编译。

### 2) exceptions 文件现状（623 行）

当前单文件混合了多个职责域：

- `try/except/finally` 基础匹配语义
- 跨函数栈帧的 unwind 与异常覆盖规则
- `StopIteration` 与 `for` 迭代交互
- 异常对象格式化与具体报错文本

去重初步核查结果：不存在“脚本输入 + 断言输出 + 语义目标”完全相同的独立 `TEST_F`。

### 3) functions 文件现状（635 行）

当前单文件同时承载多个职责域，包含：

- 默认参数/关键字参数绑定
- `*args/**kwargs` 定义侧打包语义
- 调用侧 `*`/`**` 展开与合并
- 递归/算法类集成脚本
- 函数调用异常路径

去重初步核查结果：不存在独立 `TEST_F` 级别 100% 重复；存在“同一测试内多次调用得到相同数值”的情况，但脚本调用形态不同，不属于“完全重复用例”。

## Proposed Changes

### A. exceptions 拆分为 4 个职责文件

1. 新增 `test/unittests/interpreter/interpreter-exceptions-trycatch-unittest.cc`
   - 收纳基础 `try/except/finally` 与匹配语义（如类型匹配、tuple 匹配、re-raise、raise instance/from）。
   - 原因：将“异常匹配机制”与“迭代/格式化/跨帧传播”解耦，降低单文件认知负担。

2. 新增 `test/unittests/interpreter/interpreter-exceptions-unwind-unittest.cc`
   - 收纳跨函数传播、finally 顺序、except/finally 中再抛异常等 unwind 语义。
   - 原因：这组测试面向解释器异常展开主链，具备明确单一职责。

3. 新增 `test/unittests/interpreter/interpreter-exceptions-iteration-unittest.cc`
   - 收纳 `StopIteration` 与 `for` 循环交互相关测试。
   - 原因：迭代协议相关语义与一般异常控制流职责不同，应独立维护。

4. 新增 `test/unittests/interpreter/interpreter-exceptions-format-unittest.cc`
   - 收纳 `BaseException str/repr`、pending exception 文本格式、除零报错文案测试。
   - 原因：报文/格式化属于“错误可观测性”职责，便于后续改文案时定向回归。

5. 删除原 `interpreter-exceptions-unittest.cc`。
   - 原因：避免重复编译与职责回流。

### B. functions 拆分为 5 个职责文件

1. 新增 `test/unittests/interpreter/interpreter-functions-args-binding-unittest.cc`
   - 收纳默认参数、关键字参数、混合传参、方法调用注入 self 等“绑定规则”测试。

2. 新增 `test/unittests/interpreter/interpreter-functions-varargs-def-unittest.cc`
   - 收纳定义侧 `*args/**kwargs` 打包行为（含空包、命名参数共存、业务脚本）。

3. 新增 `test/unittests/interpreter/interpreter-functions-call-unpack-unittest.cc`
   - 收纳调用侧 `*tuple` / `**dict` / 多 `**` merge 测试。

4. 新增 `test/unittests/interpreter/interpreter-functions-recursion-unittest.cc`
   - 收纳 `FibRecursion`、`MergeSortRecursion` 等递归算法场景。

5. 新增 `test/unittests/interpreter/interpreter-functions-call-errors-unittest.cc`
   - 收纳 `FunctionCallErrors` 相关异常路径测试。
   - 采用“多个独立 `TEST_F`”替代单测内多段脚本串行断言，保持单测单职责与失败定位性。

6. 删除原 `interpreter-functions-unittest.cc`。
   - 原因：避免重复编译与职责回流。

### C. “完全重复”治理策略（严格 100%）

1. 判定标准（执行时逐条核对）：
   - Python 脚本输入完全一致；
   - 断言期望（输出序列/异常子串）完全一致；
   - 语义目标无差异。

2. 处理策略：
   - 仅删除满足上述 3 条的用例；
   - 对“近重复但脚本形态不同”的用例保留（例如同值输出但覆盖不同调用路径）。

3. 预期结果：
   - 本轮很可能无独立 `TEST_F` 被删除（因当前初查未发现 100% 重复），但会通过拆分与 `FunctionCallErrors` 粒度下沉提升可维护性。

### D. 构建文件变更

更新 `test/unittests/BUILD.gn`：

- 移除：
  - `./interpreter/interpreter-exceptions-unittest.cc`
  - `./interpreter/interpreter-functions-unittest.cc`
- 增加上述 9 个新源文件路径（4+5）。

## Assumptions & Decisions

- 保持现有测试框架与 fixture：继续使用 `TEST_F(BasicInterpreterTest, ...)`，不引入新 fixture。
- 保持测试语义不变：仅进行文件级重组与必要重命名，不改变断言行为。
- 不主动做“近重复合并参数化”：遵循用户“严格仅删 100% 重复”的决策。
- 命名沿用仓库现有风格：`interpreter-<domain>-unittest.cc`。

## Verification Steps

1. 静态校验
   - 检查新旧文件与 `BUILD.gn` 一致性，确认无遗漏/重复源文件。

2. 构建校验
   - 仅构建 `test/unittests:ut`，确保新增源文件均可编译链接。

3. 测试校验（最小充分）
   - 执行 `ut` 中 interpreter 相关测试（可用 gtest filter 聚焦新拆分文件对应套件）。
   - 重点验证：
     - 异常控制流测试全部通过；
     - 函数参数绑定/展开/递归/错误路径测试全部通过；
     - 无重复注册导致的测试名冲突。

4. 回归结论记录
   - 汇总“删除了哪些 100% 重复用例（若无则明确写无）”；
   - 汇总拆分后文件清单与各自职责边界，作为后续维护基线。
