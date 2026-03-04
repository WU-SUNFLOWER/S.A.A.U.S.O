# S.A.A.U.S.O：`IsPyXxx` 从 exact 迁移到 like 的分阶段架构迁移计划

## 1. 背景与目标

* 目标一：将 `IsPyString/IsPyList/IsPyDict/IsPyTuple` 的语义统一升级为 **like**（按 `native_layout_kind` 判定）。

* 目标二：补充 `IsPyStringExact/IsPyListExact/IsPyDictExact/IsPyTupleExact`，承载“严格 klass 身份”场景。

* 目标三：在不破坏关键语义边界的前提下，让上层代码默认获得子类兼容能力，并与 V8 风格的“表示兼容优先”判定思路对齐。

## 2. 迁移总原则（必须遵守）

* 语义分层：

  * `IsPyXxx`：表示层可安全操作（like）。

  * `IsPyXxxExact`：身份层严格判定（exact）。

* 保守边界：涉及 Python 语言协议、字节码严格约束、报错文案边界的调用点优先保持 exact。

* 渐进式推进：先迁移低风险“布局驱动”路径，再迁移高风险边界路径。

* 全程可回归：每个阶段都要有独立可验证检查点，禁止跨阶段批量提交后再统一排错。

## 3. 代码改造分阶段计划

### 阶段 A：基础 API 重构（只改定义，不改调用语义）

1. 在 `py-object.h/.cc` 中调整四组宏生成逻辑：

   * 现有 `IsPyString/IsPyList/IsPyDict/IsPyTuple` 改为调用对应 like 判定入口。

   * 新增 `IsPyStringExact/IsPyListExact/IsPyDictExact/IsPyTupleExact`，保留原“klass 指针相等”逻辑。
2. 统一对齐声明与实现命名，保证 `Handle`/`Tagged` 两套重载均存在。
3. 补齐头文件注释，明确“like vs exact”语义边界与使用建议。

阶段完成标准：

* 编译通过。

* 未修改调用点语义前提下，代码可成功链接。

### 阶段 B：关键边界点显式切换为 Exact（防止语义漂移）

将下列强约束路径显式改用 `IsPyXxxExact`（即便 `IsPyXxx` 已变 like）：

1. `exec` 相关 `globals/locals` 严格 dict 检查路径。
2. 字节码调用协议中 `kwargs`/`DICT_MERGE` 的严格 dict 路径。
3. `str/tuple` 构造器“传入同类型对象直接返回自身”的 identity 快路径。
4. `__str__` 返回值严格类型校验路径。
5. 其他依赖“真内建类型身份”的显式语义守卫（例如随机模块中要求真 list 的入口）。

阶段完成标准：

* 关键边界行为与现状保持一致（包括错误类型与主要报错文案不退化）。

### 阶段 C：通用路径统一收敛到 Like（释放迁移收益）

1. 将布局兼容路径统一收敛为 like 判定（若未自动受益则显式调整）：

   * iterable/unpack/truthiness 等运行时公共 helper。

   * 仅依赖容器布局与基础协议的优化分支。
2. 清理重复判定：删除“先 `IsPyXxx` 后 `IsXxxLike`”的冗余逻辑。

阶段完成标准：

* 子类对象在通用路径上的兼容行为明显提升，且不引入新崩溃与明显语义回退。

### 阶段 D：文档与规范收敛

1. 更新 `AGENTS.md` 与相关类型头文件注释：

   * 明确 `IsPyXxx` 新语义（like）。

   * 明确 `IsPyXxxExact` 使用场景。
2. 如有必要，在对象系统章节增加“迁移后判定选择决策表”。

阶段完成标准：

* 新接手开发者可仅靠文档正确选择 API，不依赖口头约定。

## 4. 验证与回归策略

### 4.1 编译与静态检查

* `out/debug` 或现有主开发构建配置全量编译通过。

* 保证无新增告警（项目开启严格告警即错误）。

### 4.2 单测回归

* 运行 `ut` 全量单测。

* 重点关注：

  * `objects`：四大容器与子类化相关用例。

  * `interpreter`：`CALL_FUNCTION_EX`、`DICT_MERGE`、`exec`、`__str__` 相关链路。

  * `runtime`：truthiness / iterable helper 相关路径。

### 4.3 增量用例补充（必要时）

* 新增或补齐以下回归测试：

  * `IsPyXxx` 对子类返回 true；

  * `IsPyXxxExact` 对子类返回 false；

  * `str/tuple` identity fast-path 仅 exact 命中；

  * `exec`/kwargs 边界继续保持严格语义。

## 5. 交付拆分与提交建议

* 提交 1（阶段 A）：API 与注释重构。

* 提交 2（阶段 B）：关键边界改为 `Exact`。

* 提交 3（阶段 C）：通用路径收敛与冗余清理。

* 提交 4（阶段 D）：文档/测试收尾。

## 6. 风险清单与缓解

* 风险：边界路径被误切到 like，导致语言语义放宽。

  * 缓解：阶段 B 先行 + 回归测试锁定。

* 风险：调用点过多导致漏改。

  * 缓解：按符号检索生成迁移清单，逐项勾销。

* 风险：文档滞后造成后续误用。

  * 缓解：阶段 D 强制更新规范与头文件注释。

## 7. 执行顺序（确认后立即实施）

1. 先完成阶段 A（API 改造）并编译检查。
2. 立即执行阶段 B（关键边界 exact 化）并跑重点单测。
3. 再执行阶段 C（通用路径 like 收敛）并跑全量单测。
4. 最后完成阶段 D（文档与补测）并给出迁移结果清单。

