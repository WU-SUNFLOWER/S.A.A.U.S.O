# dict：runtime 与 PyDict 原语使用边界与后续治理计划

## 目标

1. 在代码中写清楚“何时使用 runtime dict 语义入口、何时使用 PyDict 原语（fallible API）”，避免后续维护者/AI 助手误用旧 API 或在错误层级实现语义。
2. 基于当前 dict 改造进展，给出下一步 API 收敛/替换与架构治理的可执行建议清单。

## 约束与原则

* 说明文字放在对外可见且最接近调用者的接口处；优先选择 `src/runtime/runtime-py-dict.h`，因为它是“语义入口”的声明面。

* 说明内容必须覆盖：

  * runtime 与 PyDict 的职责边界（语义 vs 原语）

  * “哪些场景必须用 fallible API 并传播异常”

  * interpreter 为何仍可直接调用 PyDict 原语（因为其语义不同于 `d[key]`，但必须使用 fallible API）

* 不改变现有行为，只做文档化（除非发现明显缺陷）；文档变更后需通过编译/单测验证。

## 变更计划（将执行的代码修改）

### 1) 在 runtime-py-dict.h 增加“使用边界说明”

文件：`e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-dict.h`

在 `Runtime_DictGetItem/SetItem/DelItem` 声明附近新增一段说明（块注释/文档注释均可），包含以下要点：

* **runtime（语义入口）适用场景**

  * 当调用方需要 Python 语言层语义：例如 `d[key]` 未命中抛 `KeyError`、删除未命中抛 `KeyError`、未来需要统一错误消息/repr 时。

  * 当存在多个调用点需要保持一致行为时（klass 虚方法、builtins、runtime helper 等）。

* **PyDict（原语）适用场景**

  * 当调用方实现的是“更高层/更特化的解释器语义”，且不等价于 `d[key]`：例如 `LOAD_NAME/LOAD_GLOBAL` 的 locals→globals→builtins 链式查找（未命中继续，最终 `NameError`）。

  * 这类场景仍必须使用 `GetTaggedMaybe/Put/Remove/ContainsKey` 并在失败时立即传播 pending exception（避免把 `__hash__/__eq__` 异常误判为未命中）。

* **明确禁止/避免**

  * 在 runtime/builtins/interpreter 中用 `PyDict::Get/GetTagged/Put/Contains/Remove` 旧 API 处理“可能触发 `__hash__/__eq__`”的 key；这些旧 API 仅允许用于可证明不会抛的 key（典型为 interned `PyString`）。

* **推荐用法模式**

  * runtime/builtins：`RETURN_ON_EXCEPTION_VALUE/ASSIGN_RETURN_ON_EXCEPTION` 搭配 `*Maybe`。

  * interpreter：`GOTO_ON_EXCEPTION`/显式 `goto pending_exception_unwind` 搭配 `*Maybe`。

验收标准：

* 说明文字准确、可读，并能指导新贡献者在 “runtime vs PyDict” 之间作出一致决策。

* 不引入风格冲突；文档段落尽量简洁（避免长篇大论）。

### 2) 验证

* 运行现有 unit tests（至少包含 `*Dict*` 过滤与全量 `ut.exe`）确保无回归。

* 检查 IDE 诊断（无新增编译错误/告警）。

## 下一步治理建议（本轮只给建议，不强制立即实施）

### A. “旧 API 使用”守门机制（强烈推荐）

目标：防止回归到“null == 未命中/异常”二义性。

* 在 `PyDict` 旧 API（`Get/GetTagged/Put/Contains/Remove`）上增加更强的可见性约束（择一）：

  1. 通过命名强调危险性（例如 `GetTaggedUnsafe`），并让旧名字转发到新名字（会是一次较大重命名）。
  2. 通过注释/文档明确仅限“不会抛的 key”，并在 runtime/builtins/interpreter 层建立 grep/presubmit 检查阻止新代码继续使用旧 API（实现成本低、收益高）。
  3. 在 debug 构建中增加断言（例如检测 key 类型为 `PyString` 或检测 isolate 无 pending exception 的使用模式），避免误用悄悄上线。

### A2. “最终移除旧 API”的分轮路线图（建议作为终局目标）

背景：旧 API（`Get/GetTagged/Put/Contains/Remove`）的主要问题是无法表达三态（命中/未命中/异常），容易在 key 触发 `__hash__/__eq__` 异常时造成误判与异常覆盖。当前仓库已引入 fallible API（`*Maybe`）并在关键链路完成迁移；后续可以按轮次推进，最终将旧 API 彻底移除。

考虑到当前开发团队规模较小（1 人 + AI 工具），不建议做大规模“批量重命名”工程；路线图以“弃用标注 + 分轮替换 + 最终删除”为主，保证收益与成本匹配。

**第 0 轮（立即）——宣布弃用 + 防回归**

* 在 `AGENTS.md` 与关键头文件/接口处明确声明旧 dict API 已废弃；新代码默认不得引入旧 API（除非能证明 key 永不抛且无异常传播需求）。

* 为 runtime/builtins/interpreter 建立守门策略：代码审查规则 + grep/presubmit（可选）+ debug 断言（可选）。

* 验收：新增代码不再出现旧 API；已有高风险路径全部使用 `*Maybe` 并正确传播异常。

**第 1 轮（本阶段）——替换“可疑 key”路径**

* 范围：runtime/builtins/interpreter 内所有 key 可能来自用户代码或非 `PyString` 的 dict 访问/写入/删除。

* 做法：全部替换为 `GetTaggedMaybe/Put/Remove/ContainsKey` 或 runtime 语义入口，并强制在失败时立刻传播 pending exception。

* 验收：grep 显示这些目录下不再存在旧 API 的“可疑 key”用法；相关单测覆盖 `__hash__/__eq__` 异常传播。

**第 2 轮——扩展 runtime 语义入口，进一步变薄 builtins/klass**

* 目标：把 `dict.get/setdefault/pop` 等语义集中到 runtime 入口，builtins/klass 只做参数整理与转发。

* 做法：新增 `Runtime_DictGet/Runtime_DictSetDefault/Runtime_DictPop` 等，并迁移 builtins 实现。

* 验收：builtins 中 dict 相关实现不再自行处理“未命中抛 KeyError/默认值”等语义分支；语义一致性集中在 runtime。

**第 3 轮——替换“安全 key”的旧 API**

* 范围：内部系统使用的 dict（例如 properties、string-table key、bootstrap dict），key 可证明为 interned `PyString`。

* 做法：在具备异常传播通道的函数中优先替换为 `*Maybe`；对于没有传播通道的初始化/void 路径，允许继续保留旧 API，但必须在注释里标注“仅限 PyString key”。

* 验收：旧 API 的使用面显著收缩，剩余调用点均有清晰的“安全使用”说明。

**第 4 轮——移除旧 API**

* 前置条件：仓库中不存在需要旧 API 的不可替代场景，或已提供等价替代（例如 `TryGet` 风格 out-parameter、或 `Maybe` 返回）。

* 做法：删除旧 API 声明与定义，统一使用 fallible API 或 runtime 语义入口。

* 验收：全量构建与 UT 通过；无新增性能回退（如有性能顾虑，可在移除后再补充 PyString 快路径）。

### B. runtime 语义入口继续扩展（按需）

当前已有 `Runtime_DictGetItem/SetItem/DelItem` 与 `Runtime_MergeDict`，后续可按语义聚合继续补齐：

* `Runtime_DictGet(dict, key, default)`：对应 `dict.get` 的语义入口，便于 builtins 统一实现。

* `Runtime_DictSetDefault(dict, key, default)`：对应 `setdefault`，确保异常传播与插入语义一致。

* `Runtime_DictPop(dict, key, default?)`：对应 `pop`，集中 KeyError/默认值语义与异常传播。

收益：builtins 实现变薄，语义集中，后续对齐 CPython 更容易。

### C. interpreter 层的“语义型 helper”可选收敛（谨慎）

对 `LOAD_NAME/LOAD_GLOBAL` 这类语义不等价于 `d[key]` 的场景，不建议强行改成 runtime `DictGetItem`（会引入 KeyError→NameError 的转换复杂度）。
但可以新增“解释器专用语义 helper”（例如 `Runtime_LookupNameChain(locals, globals, builtins, key)` 返回三态），让 interpreter 代码更短更一致。
这属于可选项：收益是可读性，风险是接口膨胀。

### D. 性能与正确性专项（后续）

* 当前 `Remove` 采用“重建 rehash”实现，简单且正确，但可能在频繁删除场景性能一般；后续可考虑引入 tombstone 或更接近 CPython 的 dict 删除策略（需要额外的 rehash/探测不变量与测试）。

* 可以为 `PyString` key 做专用快路径（例如已缓存 hash、或类型特化比较），但应在语义正确性稳定后再做。

