# interpreter-custom-class-basics-unittest 进一步拆分治理计划

## Summary
- 目标：评估并治理 `test/unittests/interpreter/interpreter-custom-class-basics-unittest.cc`（约 595 行、22 个测试）继续膨胀带来的维护风险。
- 结论：存在进一步膨胀劣化风险，需继续拆分。
- 用户决策：采用“四文件拆分 + 完全迁移并删除原文件 + 仅做语义完全重复去重”。

## Current State Analysis
- 当前文件承载 6 类主题：基础建类/实例化、dunder 协议、继承与类型关系、属性查找异常、方法绑定语义、`__build_class__` 错误分支，主题耦合较高。  
  参考：[interpreter-custom-class-basics-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-custom-class-basics-unittest.cc)
- custom-class 体系已分为 `construct/mro/builtin-bridge`，但 basics 仍是聚合热点，继续新增用例将放大冲突与评审上下文切换成本。  
  参考：[BUILD.gn:L42-L45](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/BUILD.gn#L42-L45)
- `BUILD.gn` 当前单点登记该文件，迁移影响面集中在 `sources` 条目更新。

## Proposed Changes

### 1) 以主题再拆分为四个文件（完全迁移）
- 新增 `test/unittests/interpreter/interpreter-custom-class-core-unittest.cc`
  - 放置“基础建类/实例化/继承/isinstance”语义。
- 新增 `test/unittests/interpreter/interpreter-custom-class-dunder-unittest.cc`
  - 放置 `__add__/__len__/__getitem__/__setitem__/__delitem__/__str__/__repr__`。
- 新增 `test/unittests/interpreter/interpreter-custom-class-attribute-binding-unittest.cc`
  - 放置 `__getattr__/__setattr__`、方法绑定、类/实例可调用属性绑定语义。
- 新增 `test/unittests/interpreter/interpreter-custom-class-build-class-unittest.cc`
  - 放置 `__build_class__` 相关成功/错误分支探针。

### 2) 严格去重（仅语义完全重复）
- 只移除“覆盖点完全一致、断言信息完全等价”的重复用例。
- 不合并相近场景，不改变错误断言语义，不放宽异常匹配强度。

### 3) 构建登记与清理
- 更新 `test/unittests/BUILD.gn`：
  - 移除 `./interpreter/interpreter-custom-class-basics-unittest.cc`
  - 新增上述 4 个文件的 `sources` 条目。
- 删除原文件 `interpreter-custom-class-basics-unittest.cc`（完全迁移策略）。

### 4) 一致性约束
- 所有迁移后用例保持：
  - `TEST_F(BasicInterpreterTest, ...)`
  - `RunScript/RunScriptExpectExceptionContains`
  - `kInterpreterTestFileName`
- 保持测试命名唯一，避免 suite + case 冲突。

## Assumptions & Decisions
- 决策1：本轮仅做测试组织治理，不修改 runtime/object 实现逻辑。
- 决策2：允许严格去重，但不会做“相近场景合并”。
- 决策3：拆分后保持 custom-class 系列命名风格与当前目录一致（`interpreter-custom-class-*-unittest.cc`）。

## Verification
- 构建验证：
  - `.\depot_tools\gn.exe gen out/ut --args="is_asan=true"`（如已有可跳过）
  - `.\depot_tools\ninja.exe -C out/ut ut`
- 定向回归：
  - `.\out\ut\ut.exe --gtest_filter="*CustomClass*:*BuildClass*:*GetAttr*:*Method*"`
- 验收标准：
  - 原文件已删除，四个新文件已收录到 `BUILD.gn`
  - 迁移后 custom-class basics 语义测试全部通过
  - 无新增编译错误与诊断错误
