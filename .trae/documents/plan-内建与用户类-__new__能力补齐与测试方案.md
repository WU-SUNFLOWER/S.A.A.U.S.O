# 内建类型与用户类 `__new__` 能力补齐与验证计划

## 1. 目标

* 评估并补齐 S.A.A.U.S.O 中“将内建类型 `NewInstance` 暴露为 Python 世界 `__new__`”的能力，使其与当前 `InitInstance -> __init__` 的桥接策略一致。

* 评估并完善“用户自定义类定义 `__new__`”的行为支持；若已支持，则补齐测试证明其语义。

* 为上述能力新增解释器端到端单元测试，覆盖正向路径与关键错误分支。

## 2. 已确认现状（基于当前源码）

* 目前内建方法安装里，`object/list/dict` 仅安装了 `__init__`，未安装 `__new__` 到类字典。

* `type.__call__ -> Runtime_NewObject -> Klass::NewInstance/InitInstance` 主链路已存在。

* 动态类创建时，`KlassVtable::UpdateOverrideSlots` 会根据“当前类字典中是否存在魔法方法”将 slot 覆盖为 trampoline，理论上支持 `__new__` 与 `__init__` 覆盖分发。

* 现有测试对 `__init__` 桥接覆盖较充分，但 `__new__` 桥接与用户类 `__new__` 语义覆盖不足。

## 3. 实施范围与改造策略

### 3.1 内建 `__new__` 暴露能力补齐

* 在以下内建方法集合中新增 `New` builtin 方法并注册为 `__new__`：

  * `PyObjectBuiltinMethods`

  * `PyListBuiltinMethods`

  * `PyDictBuiltinMethods`

* `New` 方法语义与现有 `Init` 桥接保持一致风格：

  * 支持“实例路径”（`self != null`）与“descriptor 路径”（`self == null`）。

  * descriptor 路径下从 `args[0]` 读取接收者（类型对象），并构造 tail 参数转发。

  * 参数不足时抛出与 CPython 风格对齐的 `TypeError`（descriptor needs an argument）。

  * 最终调用对应 `Klass::NewInstance(...)`，而非直接分配对象。

* 保持最小侵入，不改动现有 `Runtime_NewObject` 调度顺序与 `__init__` 返回值检查语义。

### 3.2 用户自定义类 `__new__` 语义补全（若存在缺口）

* 先按现状验证以下语义是否完整：

  * 用户类定义 `__new__` 时，实例化是否优先触发 `__new__`。

  * `__new__` 返回“非本类实例”时，`__init__` 是否仍被调用（应按当前实现明确行为，并通过测试固化）。

  * `object.__new__(Cls, ...)`/`list.__new__(Cls, ...)`/`dict.__new__(Cls, ...)` descriptor 调用是否可达。

* 若验证发现缺口，再进行最小修复：

  * 优先修复在 builtins bridge 层；

  * 仅在必要时修复 trampoline 或 `Runtime_NewObject` 参数规范化问题（例如空参数句柄转换）。

## 4. 代码改造步骤

1. 在 `builtins-py-object-methods.h/.cc` 增加并注册 `__new__` 桥接方法。
2. 在 `builtins-py-list-methods.h/.cc` 增加并注册 `__new__` 桥接方法。
3. 在 `builtins-py-dict-methods.h/.cc` 增加并注册 `__new__` 桥接方法。
4. 复核 bridge 方法中的参数拆分与错误分支文案，保持与 `__init__` bridge 一致的实现模式。
5. 若测试暴露用户类 `__new__` 调度缺口，按最小原则补丁到对应运行时路径（优先 bridge，其次 trampoline/runtime）。
6. 保持代码风格与现有文件约定一致，不引入额外架构分支。

## 5. 单元测试计划

### 5.1 新增/补充测试文件

* 主要在 `test/unittests/interpreter/interpreter-custom-class-unittest.cc` 补充测试用例（与现有 `__init__`/class 语义测试集中放置）。

* 如需与构造器语义集中管理，再补充到 `interpreter-builtins-constructor-unittest.cc`（仅在内容更贴近 builtins constructor 时）。

### 5.2 计划新增测试点

* 内建 `__new__` descriptor 可调用：

  * `f = list.__new__; x = f(list)` 可成功创建 list 实例。

  * `f = dict.__new__; x = f(dict)` 可成功创建 dict 实例。

  * `f = object.__new__; x = f(SomeClass)` 可返回 `SomeClass` 实例。

* 内建 `__new__` 参数错误分支：

  * 无接收者时报 descriptor needs an argument。

  * 错误接收者类型时抛出类型相关错误（以当前实现实际行为为准并固化）。

* 用户类 `__new__` 支持性验证：

  * `class A: def __new__(cls): ...` 路径可执行并影响构造结果。

  * `__new__` + `__init__` 的调用顺序与结果行为可观测并稳定。

  * 继承场景下 `Base.__new__` / `Derived.__new__` 分派符合当前 MRO + vtable 规则。

## 6. 验证与回归

* 构建并运行 `ut`，至少覆盖以下范围：

  * 新增解释器测试用例全部通过。

  * 现有 `interpreter-custom-class-unittest` 相关历史用例不回归。

* 若时间允许，追加一次全量 `ut` 回归，确认无连带破坏。

* 使用诊断检查确保无新增编译错误/静态诊断问题。

## 7. 交付结果

* 代码侧：

  * 内建 `__new__` 已可在 Python 层通过 descriptor 访问与调用（至少覆盖 object/list/dict）。

  * 用户类 `__new__` 支持状态被明确：若有缺口已修复；若已支持则通过测试给出可执行证据。

* 测试侧：

  * 新增用例可稳定复现并验证语义，且通过 CI/本地 `ut`。

* 文档侧：

  * 在最终说明中给出“当前语义与 CPython 的一致点/差异点”摘要，便于后续继续对齐。

