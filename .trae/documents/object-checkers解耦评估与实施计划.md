# object-checkers 解耦评估与实施计划

## 1. 结论

这是一个**值得推进**的方向，且对当前代码基线是“低风险、长期收益高”的演进：

* `py-object.h/.cc` 同时承载“对象核心行为 + 大量类型判定工具”，职责已经偏重。

* checker 的语义已经独立成体系（like/exact、klass-based、特殊对象判定），抽离后更利于维护与扩展。

* 抽离后可降低 `py-object.cc` 的 include 压力，减少后续改动的冲突面与编译耦合。

但建议采用“**平滑迁移**”而非一次性大爆炸：

* 对外函数名保持不变（`IsPyXxx` / `IsPyXxxExact`），先迁移实现位置，再收敛头文件依赖。

* 先保证 ABI/API 与现有调用方无感，再做 include 结构优化。

## 2. 目标与非目标

### 目标

1. 将 checker 的声明与实现从 `py-object.h/.cc` 解耦到独立模块（如 `object-checkers.h/.cc`）。
2. 保持现有调用点无需改名、无需语义调整。
3. 明确 checker 的分层与规则：like、exact、special-case（如 Smi/True/False/GC-able）。

### 非目标

1. 不改变任何 checker 的语义结果。
2. 不同时引入新的类型系统或模板元编程重构。
3. 不在本次改造中调整 `PyObject` 多态入口的行为。

## 3. 实施步骤

### Step A：建立新模块骨架

1. 新增 `src/objects/object-checkers.h`：

   * 承载全部 `IsPyXxx` 与 `IsPyXxxExact` 声明。

   * 保留 `Tagged<PyObject>` 与 `Handle<PyObject>` 双重重载。

   * 将 checker 宏（若仍需）局限在该头文件内部，避免污染 `py-object.h`。
2. 新增 `src/objects/object-checkers.cc`：

   * 迁移 `py-object.cc` 中“类型判断工具函数”整段实现。

   * 保留当前 like/exact 规则与 `NativeLayoutKind` 判定逻辑。

### Step B：做兼容层与最小侵入改造

1. 在 `py-object.h` 中移除 checker 的原始声明块，改为：

   * `#include "src/objects/object-checkers.h"`，对外维持同名 API 可见性。
2. 在 `py-object.cc` 中删除 checker 实现段与相关宏定义，仅保留 `PyObject` 核心方法实现。
3. 处理 include 依赖：

   * `object-checkers.cc` 引入原 checker 所需的 `*-klass.h`、`isolate.h` 等。

   * `py-object.cc` 减少不再需要的 klass 头依赖。

### Step C：构建系统与导出边界

1. 更新 `BUILD.gn`，确保 `object-checkers.cc` 被编译并与现有对象模块链接。
2. 校验 `src/objects/objects.h` 是否需要显式聚合 `object-checkers.h`：

   * 若现有代码依赖“只 include objects.h 即可用 checker”，则补齐聚合导出。

   * 若不依赖，则保持最小暴露，避免不必要耦合。

### Step D：调用点一致性检查

1. 全仓扫描确认：

   * 无残留 `py-object.cc` 内部私有 checker 实现引用。

   * 所有 `IsPyXxx`、`IsPyXxxExact` 均由新模块提供定义。
2. 检查 ODR/重复定义风险：

   * 确保声明只在 `object-checkers.h`，定义只在 `object-checkers.cc`。

### Step E：测试与回归

1. 编译验证：`gn gen` + `ninja`（ut target）。
2. 单测验证：执行 `ut.exe` 全量。
3. 语义重点回归：

   * 已有 like/exact 单测保持通过。

   * exec/DICT\_MERGE/CALL\_FUNCTION\_EX/__str__/shuffle 等 exact 边界用例保持通过。

### Step F：文档更新

1. 更新 `AGENTS.md`：

   * 明确 checker 已拆分至 `object-checkers.*`。

   * 保持 “IsPyXxx=like / IsPyXxxExact=exact” 约定描述不变。
2. 若项目有架构文档入口，补充“checker 所属模块职责边界”。

## 4. 风险与缓解

1. **头文件循环依赖风险**

   * 缓解：在 `object-checkers.h` 仅使用前置声明 + 必要基础头，避免引入重量级头。
2. **链接缺符号风险（漏加 cc 到构建）**

   * 缓解：先做最小构建验证，再跑全量 UT。
3. **语义漂移风险（迁移时误改逻辑）**

   * 缓解：严格“搬运不改义”，并以现有 like/exact UT 作为守护线。

## 5. 验收标准

1. `py-object.cc` 不再包含 checker 实现，仅保留 `PyObject` 核心能力。
2. checker API 统一由 `object-checkers.h/.cc` 提供，外部调用无需改名。
3. 全量 UT 通过，且 like/exact 相关行为无回归。
4. 文档完成同步，明确新的模块边界与语义约定。

