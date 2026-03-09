# ConstructInstance 拆分为 New/Init 的架构治理计划

## 1. 目标与范围

### 1.1 目标

* 将当前 `ConstructInstance(args, kwargs)` 单一职责拆分为：

  * `NewInstance(args, kwargs)`：负责“分配/构造原始对象”（对应 `tp_new` 语义边界）。

  * `InitInstance(instance, args, kwargs)`：负责“初始化调用与校验”（对应 `tp_init` 语义边界）。

* 保持 `Runtime_NewObject` 作为统一门面，调整其内部调度顺序为：

  * `new` → `init` → 返回对象。

* 对齐现有异常传播模型（`MaybeHandle` + pending exception），不改变现有外部可观察错误语义。

### 1.2 本次改动范围

* 构造协议与默认实现：

  * `src/objects/klass.h`

  * `src/objects/klass.cc`

  * `src/objects/klass-default-vtable.cc`

* 运行时门面调度：

  * `src/runtime/runtime-reflection.h`

  * `src/runtime/runtime-reflection.cc`

* 重点内建可继承类型：

  * `src/objects/py-list-klass.h/.cc`

  * `src/objects/py-dict-klass.h/.cc`

  * `src/objects/py-tuple-klass.h/.cc`

  * `src/objects/py-string-klass.h/.cc`

* 同步补齐其他已覆写 `Virtual_ConstructInstance` 的类型：

  * `src/objects/py-smi-klass.h/.cc`

  * `src/objects/py-float-klass.h/.cc`

### 1.3 非目标

* 不引入独立 `tp_alloc` 概念（继续复用 `Factory`）。

* 不修改 Python 语义层面规则（参数个数、kwargs 允许性、错误文案保持现状）。

* 不在本次引入新的 Python 级 `__new__` 特殊行为（保持当前系统语义边界）。

## 2. 设计方案

### 2.1 Klass vtable 与虚函数拆分

* 在 `KlassVTable` 中新增两个槽位：

  * `new_instance`

  * `init_instance`

* 在 `Klass` 中新增对应调度方法：

  * `MaybeHandle<PyObject> NewInstance(...)`

  * `Maybe<void> InitInstance(...)`

* 保留 `ConstructInstance(...)` 作为兼容外壳（内部调用 `new` + `init`），用于平滑迁移已有调用点与后续分批演进。

### 2.2 默认实现策略

* 默认 `Virtual_Default_NewInstance`：

  * 继续按现有默认路径分配对象（保持 native/heap type 路径不变）。

* 默认 `Virtual_Default_InitInstance`：

  * 复用现有“查找并调用 `__init__`”流程。

  * 若未找到 `__init__`，返回成功。

* 默认 `Virtual_Default_ConstructInstance`：

  * 调用 `new` 与 `init`，以保证旧路径可用且行为一致。

### 2.3 Runtime\_NewObject 门面重构

* 将 `Runtime_NewObject(...)` 从“直接 `ConstructInstance`”切换为：

  * `obj = own_klass()->NewInstance(args, kwargs)`

  * `own_klass()->InitInstance(obj, args, kwargs)`

  * 返回 `obj`

* 同时保留 `Klass::ConstructInstance` 兼容实现，避免其他内部点位在迁移窗口期失效。

### 2.4 各类型迁移原则

* `list/dict/tuple/str`：

  * 现有 `Virtual_ConstructInstance` 的逻辑按职责拆分进 `Virtual_NewInstance` 与 `Virtual_InitInstance`。

  * 原有 exact/subclass 分支、kwargs 校验、错误文案、返回对象身份语义保持不变。

* `tuple/str` 不可变类型特殊要求：

  * 保留 `exact + 单参数 + 入参同型` 的“直接返回入参”快路径在 `new` 阶段。

  * `init` 阶段避免破坏已构造结果。

* `dict/list` 可变类型：

  * 保持当前从参数初始化内容的行为；仅重排到 `new`/`init` 边界，确保最终可观察行为不变。

* `smi/float`：

  * 将现有构造流程同步迁移为 `new/init`，保证编译期虚函数覆盖完整。

## 3. 实施步骤（按提交逻辑顺序）

1. 更新 `klass.h`：新增 vtable 槽位与 `Klass` 新接口声明，补齐默认虚函数声明。
2. 更新 `klass-default-vtable.cc`：实现默认 `new/init/construct` 三件套并绑定 vtable。
3. 更新 `klass.cc`：实现 `Klass::NewInstance/InitInstance/ConstructInstance` 调度逻辑。
4. 更新 `runtime-reflection.h/.cc`：让 `Runtime_NewObject` 显式走 `new` + `init`。
5. 迁移 `py-list-klass.h/.cc`：拆分现有构造实现并保持异常文案与参数语义。
6. 迁移 `py-dict-klass.h/.cc`：同上，特别校验 `Runtime_InitDictFromArgsKwargs` 触发点不变。
7. 迁移 `py-tuple-klass.h/.cc`：保持不可变类型快路径和复制语义。
8. 迁移 `py-string-klass.h/.cc`：保持转换/解码报错分支与 subclass 复制语义。
9. 迁移 `py-smi-klass.h/.cc` 与 `py-float-klass.h/.cc`：补全新接口实现。
10. 全仓检索清理：确保无遗漏的 `construct_instance` 旧绑定错误或虚函数签名不一致。

## 4. 验证计划

### 4.1 编译验证

* 全量构建（至少当前默认 target）确保无虚函数签名/链接错误。

* 重点检查对象系统与 runtime 相关目标文件编译通过。

### 4.2 行为回归验证

* 重点回归内建构造行为：

  * `str/tuple/list/dict` 的参数个数、kwargs 限制、错误文案。

  * 子类化场景下 `__init__` 的查找与调用路径。

  * `tuple/str` 的同型入参返回与不可变对象身份语义。

* 异常路径验证：

  * `MaybeHandle` 失败传播不吞异常。

  * `Runtime_NewObject` 在 `new` 或 `init` 失败时返回空 `MaybeHandle` 且保留 pending exception。

### 4.3 一致性检查

* 确认 `Runtime_NewObject` 仍是统一创建门面（`type_call` 风格调度点）。

* 确认 `ConstructInstance` 仅作为兼容壳，不再承载主要复杂逻辑。

## 5. 风险与缓解

* 风险：拆分时行为漂移（尤其 `str/tuple` 快路径）。

  * 缓解：先机械拆分，再以现有错误文案和返回身份为基线逐项对照回归。

* 风险：调用链并存期产生双路径不一致。

  * 缓解：`ConstructInstance` 明确委托 `new/init`，保证单一真实实现源。

* 风险：子类 `__class__` 绑定时序变化。

  * 缓解：保持当前对象创建后立即补 `__class__` 的既有时序，不在本次重排。

## 6. 交付标准

* 代码层面：

  * `Klass` 完成 `new/init/construct` 三层接口。

  * `Runtime_NewObject` 完成门面式顺序调度。

  * 6 个已覆写构造类型全部迁移到 `Virtual_NewInstance/Virtual_InitInstance`。

* 质量层面：

  * 构建通过。

  * 关键构造行为与异常语义回归通过。

  * 无遗留旧式覆盖点导致的编译或行为分叉。

