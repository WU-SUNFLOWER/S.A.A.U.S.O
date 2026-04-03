# Gemini 审查 Prompt：S.A.A.U.S.O 老生代 GC 与晋升机制开发计划

请你作为一名熟悉 V8、HotSpot、SpiderMonkey、CPython 内存管理设计的高级虚拟机工程师，严格审查我为 S.A.A.U.S.O 制定的一份开发计划。

你的任务不是直接实现代码，而是对这份计划做**技术评审**，重点判断它是否：

1. 符合一个教学型、工程型 C++20 Python VM 的现实演进路径。
2. 在架构上正确借鉴了“早期 V8 风格”的 generational GC 思路。
3. 在工程复杂度上做了合理取舍，没有过早设计。
4. 对 S.A.A.U.S.O 当前代码基线的判断是否准确。
5. 对老生代空间、promotion、remembered set、write barrier、major GC、mark-sweep、mark-compact 的先后顺序是否合理。
6. 是否遗漏了关键风险、关键测试、关键前置条件或关键设计决策。

请你输出时遵守以下要求：

- 先给出**总体结论**：这份计划是否合理，是否建议接受。
- 然后分别从以下维度审查：
  - 架构方向
  - GC 算法选型
  - promotion 设计
  - write barrier / remembered set 设计
  - OldSpace 设计
  - `MarkWord` / side metadata 方案取舍
  - major GC 风险
  - mark-compact 是否应推迟
  - 测试与验收标准
- 明确指出：
  - 哪些部分设计得好
  - 哪些部分过于理想化
  - 哪些部分还不够具体
  - 哪些部分存在明显遗漏
- 如果你不同意其中任何阶段划分，请给出**更优的阶段顺序**。
- 如果你认为第一阶段不应做 promotion，或者不应做 mark-sweep，也请明确说明原因。
- 如果你认为“仿早期 V8”这个目标表述不准确，也请给出更准确的表述方式。

请特别关注以下问题：

1. S.A.A.U.S.O 当前对象模型与 `Tagged<T>` 使用方式，是否天然更适合 non-moving old generation。
2. 在 remembered set / write barrier 尚未完全收口前，引入 promotion 是否会带来系统性风险。
3. `MarkWord` 当前已承载 klass / forwarding-address 语义，这是否意味着 side metadata 比扩展对象头更稳妥。
4. 当前已有部分语义依赖对象地址，这对未来 mark-compact 的阻碍有多大。
5. “先 OldSpace + write barrier + promotion + mark-sweep，再评估 mark-compact”这一主线是否合理。

请你在结尾输出：

- **你建议我保留的部分**
- **你建议我修改的部分**
- **你建议我补充的问题清单**
- **如果由你来重写这份计划，你会调整的前三项内容**

这是待审查的开发计划文档内容：

---

（请将 `docs/Saauso-Old-Generation-GC-Development-Plan.md` 的完整内容粘贴到这里）

---
