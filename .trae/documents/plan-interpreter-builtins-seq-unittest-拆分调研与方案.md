# interpreter-builtins-seq-unittest 拆分调研与方案

## Summary

* 目标：确认 `test/unittests/interpreter/interpreter-builtins-seq-unittest.cc` 拆分的必要性与可行性，并判断是否应超过“str/list/tuple/dict”四文件粒度。

* 结论倾向：有必要拆分，且“仅四文件”不是最优；建议在类型维度之外，再按横切主题（builtin method dispatch、subclass+GC）拆分。

## Current State Analysis

* 当前文件：`test/unittests/interpreter/interpreter-builtins-seq-unittest.cc`，约 1040 行，`TEST_F` 数量 43，主题混排（builtin 方法直调、str/list/tuple 基础行为、错误语义、子类化、GC 存活）。

* 构建登记：`test/unittests/BUILD.gn` 在 `saauso_enable_cpython_compiler` 条件内仅登记了该单文件入口（`./interpreter/interpreter-builtins-seq-unittest.cc`）。

* 现有同目录拆分风格：已存在按主题拆分的文件，如 `interpreter-builtins-constructor-unittest.cc`、`interpreter-dict-unittest.cc`、`interpreter-exceptions-unittest.cc`、`interpreter-custom-class-*.cc`。

* 关键事实：`dict` 主题已在 `test/unittests/interpreter/interpreter-dict-unittest.cc` 中独立维护，本 seq 文件几乎不承载 dict 专项。

* 技术耦合：本文件只依赖统一夹具 `BasicInterpreterTest` 与常规对象断言工具，无复杂跨测试本地共享 helper，迁移成本低。

## Proposed Changes

### 方案目标

* 将当前“大而全”文件拆为多个可维护主题文件，控制单文件规模、降低冲突面、提高定位效率与 review 可读性。

### 推荐拆分粒度（优先）

* 保留你建议的类型维度核心思想，但调整为更贴近当前内容分布的“5\~6 文件方案”：

* <br />

  1. `interpreter-builtins-method-dispatch-unittest.cc`

  * 迁移：`CallMethodOfBuiltinType`、`CallBuiltinMethodViaTypeObject*` 等“type.method(receiver)”与 descriptor 错误路径。

  * 原因：这是跨 str/list/tuple 的横切语义，不宜硬塞到某个类型文件。

* <br />

  1. `interpreter-str-unittest.cc`

  * 迁移：`find/rfind/index/split/join` 及对应错误路径 + 必要 str 基础行为。

* <br />

  1. `interpreter-list-unittest.cc`

  * 迁移：list 构造、append/insert/pop/reverse/sort、运算与迭代相关测试。

* <br />

  1. `interpreter-tuple-unittest.cc`

  * 迁移：tuple 构造、index、迭代/基础行为测试。

* <br />

  1. `interpreter-builtins-subclass-gc-unittest.cc`（推荐新增）

  * 迁移：`SubclassStr*`、`SubclassTuple*`、`*SurvivesSysgc` 等子类化与 GC 存活组合测试。

  * 原因：该类测试跨多个内建类型，且关注点是“对象模型与生命周期”而非某单类型 API。

* <br />

  1. （可选）`interpreter-list-tuple-index-propagation-unittest.cc`

  * 迁移：`ListIndexPropagatesExceptionFromEq`、`TupleIndexPropagatesExceptionFromEq`。

  * 原因：若希望把“异常传播语义”进一步集中，可独立；否则留在 list/tuple 文件即可。

### 兼容你的“四文件设想”的落地说明

* `interpreter-str-unittest.cc` / `interpreter-list-unittest.cc` / `interpreter-tuple-unittest.cc`：必要且可行。

* `interpreter-dict-unittest.cc`：仓库已存在且覆盖面较完整，不建议从 seq 再拆一个同名或重复语义文件。

### 具体修改点（若进入执行）

* 更新 `test/unittests/BUILD.gn`

  * 移除：`./interpreter/interpreter-builtins-seq-unittest.cc`

  * 新增：上述新 `.cc` 源文件条目。

* 新建并迁移测试：

  * 目标目录均为 `test/unittests/interpreter/`

  * 保持 `TEST_F(BasicInterpreterTest, <原测试名>)` 名称不变，避免语义变化。

* 删除原聚合文件（在所有测试迁移完成并编译通过后执行）。

## Assumptions & Decisions

* 决策 1：拆分以“可维护主题边界”优先，而非机械按内建类型四分。

* 决策 2：`dict` 不在本次 seq 拆分范围内扩展新文件，因为已有独立 `interpreter-dict-unittest.cc`。

* 决策 3：拆分时不改测试逻辑，仅做文件重组，降低回归风险。

* 假设：`BasicInterpreterTest` 夹具 API 稳定，迁移后无需新增共享 helper。

## Verification Steps

* 编译验证：构建 `test/unittests:ut`（启用 `saauso_enable_cpython_compiler` 配置）确保新增源文件均被编译。

* 测试验证：运行与拆分文件相关的 gtest 过滤集合，确认通过率与拆分前一致。

* 覆盖验证：对比拆分前后 `TEST_F` 名称集合，确认无漏迁移、无重复注册。

* 结构验证：确认 `test/unittests/BUILD.gn` 不再引用 `interpreter-builtins-seq-unittest.cc`，且不存在重名冲突文件。

