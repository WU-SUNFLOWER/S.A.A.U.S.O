# Gemini 审查 Prompt：S.A.A.U.S.O Phase 0 详细任务拆解

请你作为一名熟悉 V8、HotSpot、SpiderMonkey、CPython 内存管理实现的高级虚拟机工程师，严格审查我为 S.A.A.U.S.O 制定的 **Phase 0 详细任务拆解文档**。

你的任务不是直接写代码，而是从技术路线、工程可执行性、风险控制与阶段边界四个维度，对这份 Phase 0 计划做评审，并回答它是否已经足够成熟，可以立即进入执行阶段。

请你重点回答以下问题：

1. 这份 Phase 0 任务拆解是否合理，是否符合当前 S.A.A.U.S.O 的代码基线？
2. 把 Phase 0 聚焦为“架构决议冻结 + 写入路径收口 + roots 审计 + 地址语义审计 + 测试护栏”是否正确？
3. 这份文档列出的重点改造文件是否合理，是否遗漏了关键文件？
4. 把 `list / dict / function / klass / type-object` 作为第一批高风险对象是否正确？
5. 把 `factory.cc` 放在初始化窗口规则收束阶段是否合理？
6. 把 `WRITE_BARRIER` 和 remembered set 的正式启用推迟到 Phase 0 之后，是否是正确顺序？
7. 你是否同意：
   - `id()` 未来与物理地址解耦
   - 不支持 `__del__`
   - 使用 C++ `private + getter + setter` 强制写屏障接入
   - Phase 0 默认采用 side metadata / bitmap 方向
8. 这份 Phase 0 计划是否已经足够成熟，建议我**立即按计划执行**，还是应该先补充/修改某些内容再开始？

请你输出时遵守以下格式：

- **总体结论**
- **建议接受 / 建议修改后接受 / 不建议执行**
- **你认为设计得好的部分**
- **你认为仍不够具体的部分**
- **你认为遗漏的文件或风险**
- **你建议调整的执行顺序**
- **是否建议我立即开始执行**
- **如果不建议立即执行，开始前还必须补什么**

请特别关注以下审查重点：

- Phase 0 是否把“真正阻碍后续 GC 实现的工程问题”排在了前面，而不是提前进入 `OldSpace` 或 `mark-sweep` 实现。
- 文件优先级是否合理，尤其是：
  - `heap.h / heap.cc`
  - `factory.cc`
  - `py-list.*`
  - `py-dict.*`
  - `py-function.*`
  - `klass.*`
  - `py-type-object.*`
- roots 审计是否覆盖：
  - `Isolate`
  - `HandleScopeImplementer`
  - `GlobalHandles`
  - `Interpreter`
  - `FrameObject`
  - `ModuleManager`
  - `ExceptionState`
  - `StringTable`
- 地址语义审计是否覆盖：
  - 默认 `repr`
  - 默认 `hash`
  - future `id()`
  - `__del__` 缺位的架构影响
- “初始化窗口允许裸写、对象发布后必须走 setter”这一约束是否足够清晰、是否可执行。

请不要泛泛而谈，请尽量像一位真正要签字批准该阶段计划的 VM 架构评审人一样，给出明确判断。

以下是待审查文档：

1. 总体开发计划：
- `docs/Saauso-Old-Generation-GC-Development-Plan.md`

2. 上一轮技术评审报告：
- `docs/Saauso-Old-Generation-GC-Review.md`

3. 本次待审查的 Phase 0 文档：
- `docs/Saauso-Phase0-Detailed-Task-Breakdown.md`

如果你认为我现在就可以开始执行，请明确回答：

**“建议立即执行 Phase 0”**

如果你认为还不应该开始，请明确回答：

**“不建议立即执行 Phase 0，需先补充以下内容”**
