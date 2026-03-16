# interpreter-custom-class-unittest 拆分与去冗余执行计划

## Summary

* 目标：评估 `test/unittests/interpreter/interpreter-custom-class-unittest.cc` 长期膨胀风险，并在确认存在风险后完成按语义拆分。

* 用户决策：按语义拆成四个文件；完全迁移并替换原文件；仅清理“语义完全重复”的冗余用例。

* 约束：不改被测 runtime 逻辑，不改变测试语义覆盖；仅做测试组织与冗余治理。

## Current State Analysis

* 当前文件规模为 1069 行，包含 42 个 `TEST_F(BasicInterpreterTest, ...)`，主题横跨类基础能力、继承/MRO、内建桥接、`__new__/__init__/__call__` 构造语义。

* 该文件仅在 `test/unittests/BUILD.gn` 的 `sources` 中被单点登记，不被其他源码 include，拆分时的核心影响面在 BUILD 源列表维护。

* 风险评估（确认存在风险）：

  * 演进风险：对象系统持续扩展时，单文件继续堆叠会放大冲突与回归定位成本。

  * 可维护性风险：跨主题混排导致 code review 上下文切换频繁，降低审阅质量。

  * 稳定性风险：大文件中的相似用例更易引入重复覆盖，增加“改了很多但新增信息很少”的噪声。

  * 工程风险：单点文件成为热点，后续多人并行开发时更易产生合并冲突。

## Proposed Changes

### 1) 物理拆分 custom-class 测试文件（四文件）

* 新增 `test/unittests/interpreter/interpreter-custom-class-basics-unittest.cc`

  * 承载“类基础能力 + 方法绑定 + 字符串表示 + isinstance/build\_class 错误”相关用例。

  * 来源：原文件前半段基础主题测试。

* 新增 `test/unittests/interpreter/interpreter-custom-class-mro-unittest.cc`

  * 承载“单继承/菱形继承 C3 + 类型/实例沿 MRO 查找”相关用例。

  * 来源：原文件 MRO 专项测试段。

* 新增 `test/unittests/interpreter/interpreter-custom-class-builtin-bridge-unittest.cc`

  * 承载“list/dict/object 的 `__init__/__new__` bridge、receiver 校验、slot 可调用性”相关用例。

  * 来源：原文件 builtin bridge 测试段。

* 新增 `test/unittests/interpreter/interpreter-custom-class-construct-unittest.cc`

  * 承载“`__new__` 参与构造、单例/工厂/缓存、`__init__` 约束、`__call__` 分派”相关用例。

  * 来源：原文件构造与调用语义测试段。

### 2) 清理语义完全重复的冗余用例（仅限严格重复）

* 在 builtin bridge 子域内执行“严格重复治理”：

  * 合并完全镜像、仅类型名不同且断言模式一致的 pair（list/dict 对称验证）为单个组合用例，保留对两类内建类型的覆盖断言。

  * 不删除语义不完全一致的用例（例如 receiver 约束、实例路径、错误文本差异、构造链边界）。

* 保留规则：

  * 每条行为语义至少保留一个稳定代表；

  * 不修改异常断言意图，不放宽断言标准。

### 3) 替换原文件并更新构建登记

* 删除 `test/unittests/interpreter/interpreter-custom-class-unittest.cc`（用户要求“完全迁移并替换原文件”）。

* 更新 `test/unittests/BUILD.gn`：

  * 移除原 `interpreter-custom-class-unittest.cc`；

  * 新增四个拆分文件到同一 `sources += [...]` 区块，保持现有构建条件与顺序风格一致。

### 4) 稳定性与可读性保障

* 全部新文件继续使用 `BasicInterpreterTest`、`RunScript`、`RunScriptExpectExceptionContains`、`kInterpreterTestFileName`，确保 fixture 生命周期与输出捕获行为不变。

* 保持测试命名唯一，避免 suite + case 重名。

* 仅做“搬移/合并重复用例/BUILD 同步”三类变更，不引入额外功能性改动。

## Assumptions & Decisions

* 决策1：拆分粒度固定为四文件，按语义边界组织，优先降低未来膨胀速度。

* 决策2：原文件删除而非保留壳文件，减少双入口维护成本。

* 决策3：冗余清理采用保守策略，仅处理语义完全重复，不做激进压缩。

* 假设1：当前 CI/本地测试命令沿用 `out/ut` 产物与 `ut` 目标。

## Verification

* 构建验证：

  * `.\depot_tools\gn.exe gen out/ut --args="is_asan=true"`

  * `.\depot_tools\ninja.exe -C out/ut ut`

* 回归验证（聚焦 custom-class 相关）：

  * `.\out\ut\ut.exe --gtest_filter=*CustomClass*:*Mro*:*Init*:*New*`

  * 若过滤命中不足，再运行全量 `.\out\ut\ut.exe` 进行兜底确认。

* 验收标准：

  * 新增四文件全部被 BUILD 收录；

  * 原文件已移除；

  * custom-class 相关测试全部通过，且无新增编译/链接错误；

  * 冗余清理后覆盖意图不变（通过用例语义逐项对照确认）。

