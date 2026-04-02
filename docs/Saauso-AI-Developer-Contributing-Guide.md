# S.A.A.U.S.O AI Developer Contributing Guide

本文档用于约束 AI 助手在 S.A.A.U.S.O 仓库中的行为规范与交付口径，确保修改符合仓库事实、架构边界与可验证性要求。

## 1. 执行优先级与冲突处理

- **规则优先级（从高到低）**：
  1. 仓库现有实现与“能编译/能链接/能运行单测”的事实
  2. 本文档（`docs/Saauso-AI-Developer-Contributing-Guide.md`）
  3. 项目代码规范（`docs/Saauso-Coding-Style-Guide.md`）
  4. 通用最佳实践，包括但不限于：
     - SOLID 原则
     - Don't Repeat Yourself 原则
     - The Boy Scout Rule 规则
- **冲突处理流程**：
  1. 先在仓库中搜索同类代码与既有模式（优先保持局部一致性）。
  2. 若仍存在多种可行实现，选择更符合本仓库架构约束、且对现有代码扰动更小的方案。
  3. 若约束仍不明确，优先保证行为正确与可测试性，再逐步收敛风格。
- **用户指令优先**：当用户的最新明确指令与本文档冲突时，以用户指令为准；但应在交付说明中点出偏离点与潜在风险，并尽量把偏离范围限制在最小。

## 2. 必守规则（AI Checklist）

1. 修改前先在仓库中搜索同类实现与既有模式，再决定具体写法（见本文件第 1 节）。
2. 禁止在接口上传递 `PyObject*`；对外暴露与内部调用使用 `Tagged<PyObject>` 或 `Handle<PyObject>`（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
3. 栈上持有 GC-able 对象必须使用 `Handle<T>`；跨 `HandleScope` 返回必须使用 `EscapableHandleScope::Escape`（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
4. 分配 `PyObject` 派生对象禁止使用 `new`；必须优先通过 `Isolate::Current()->factory()->NewXxx(...)` 进入统一工厂路径（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
5. 不依赖构造函数写默认值：`Allocate/AllocateRaw` 不清零且不调用构造函数，默认值应在工厂函数中手工写入（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
6. 新增/重写 `Klass::vtable_` 的 slot 时必须显式指向默认实现，或确保所有调用点对 `nullptr` 可处理（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
7. `instance_size` 必须为“不可触发 GC”的纯计算；`iterate` 必须遍历对象内全部 `Tagged<PyObject>` 引用字段（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
8. `src/utils/` 严禁依赖虚拟机上层能力；不确定时先查同目录既有代码并保持依赖方向单向（见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)）。
9. 所有内部代码必须位于 `namespace saauso::internal`，并遵循代码风格指南 [Saauso-Coding-Style-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md) 的命名与注释规范（必须包含必要的简体中文注释）。
10. 新增单元测试文件后必须同步加入 `test/unittests/BUILD.gn` 的 `sources` 列表（见 [Saauso-Build-and-Test-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)）。
11. 变更完成后，交付前必须至少执行一次 `gn check`；尤其是修改 `BUILD.gn`、头文件 include、`deps/public_deps/visibility` 或新增源文件时，不得跳过该检查（见 [Saauso-Build-and-Test-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)）。

## 3. 与其他文档的分工

- 代码风格与注释规范：见 [Saauso-Coding-Style-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md)。
- VM 系统架构、依赖边界与关键协议：见 [Saauso-VM-System-Architecture.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)。
- VM 工程约束与正确性红线：见 [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)。
- 构建、测试与单测接入：见 [Saauso-Build-and-Test-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)。
- 仓库结构与阅读路线：见 [Saauso-Project-Structure-And-Reading-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Project-Structure-And-Reading-Guide.md)。
