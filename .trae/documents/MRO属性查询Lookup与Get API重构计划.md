# MRO属性查询 Lookup 与 Get API 重构计划

## 结论

该改造建议合理，且符合仓库现有的错误语义分层：

* `Lookup` 语义：查询失败不抛 Python 异常，返回 miss。

* `Get` 语义：查询失败即抛 `AttributeError`，并以空 `MaybeHandle` 向上传播。

这与仓库在 `Try/Lookup API` 与 `Fallible API` 的约定一致，可显著降低调用方误用风险并提升可读性。

## 目标与假设

### 目标

1. 将现有 API 重命名以显式表达 lookup 语义：

   * `Runtime_FindPropertyInKlassMro` → `Runtime_LookupPropertyInKlassMro`

   * `Runtime_FindPropertyInInstanceTypeMro` → `Runtime_LookupPropertyInInstanceTypeMro`
2. 新增 get 语义 API：

   * `MaybeHandle<PyObject> Runtime_GetPropertyInKlassMro(...)`

   * `MaybeHandle<PyObject> Runtime_GetPropertyInInstanceTypeMro(...)`
3. 保持现有运行时行为不回归，尤其是：

   * 查询过程中的异常仍按 `Maybe/MaybeHandle` 约定传播。

   * 只有“未命中”场景新增统一 `AttributeError` 抛错入口。

### 假设

* 你在需求中第二个 API 名写成了重复的 `Runtime_GetPropertyInKlassMro`，这里按上下文将其解释为 `Runtime_GetPropertyInInstanceTypeMro` 并落地实现。

## 影响范围（已定位）

* 声明与实现：

  * `src/runtime/runtime-reflection.h`

  * `src/runtime/runtime-reflection.cc`

* 直接调用方（需统一重命名）：

  * `src/objects/klass-default-vtable.cc`

  * `src/objects/klass-vtable.cc`

  * `src/objects/py-type-object-klass.cc`

  * `src/runtime/runtime-py-string.cc`

## 详细实施步骤

### 第一步：重命名 Lookup API（声明、定义、调用点）

1. 在 `runtime-reflection.h` 中重命名两个函数声明，注释文案同步改为 “Lookup” 语义描述。
2. 在 `runtime-reflection.cc` 中重命名两个函数定义与互相调用关系。
3. 全仓替换调用点到新函数名，确保不留旧符号。
4. 复查 `KlassVtable::UpdateOverrideSlots`、`Virtual_GetAttr`、`Virtual_Default_*` 等热点路径，确认行为仅是符号名变化，无语义变化。

### 第二步：新增 Get API（Klass MRO 与 InstanceType MRO）

1. 在 `runtime-reflection.h` 新增两个 `MaybeHandle<PyObject>` 声明，并补齐中文注释：

   * 命中：返回目标属性对象。

   * 未命中：抛 `AttributeError` 并返回空 `MaybeHandle`。

   * 查询异常：透传异常并返回空 `MaybeHandle`。
2. 在 `runtime-reflection.cc` 实现 `Runtime_GetPropertyInKlassMro`：

   * 复用 `Runtime_LookupPropertyInKlassMro` 获得三态结果。

   * `Nothing`：直接返回 `kNullMaybeHandle`（异常已存在）。

   * `true`：返回命中值。

   * `false`：构造并抛出 `AttributeError` 后返回 `kNullMaybeHandle`。
3. 在 `runtime-reflection.cc` 实现 `Runtime_GetPropertyInInstanceTypeMro`：

   * 先取 `instance` 的 `klass`，再委托 `Runtime_GetPropertyInKlassMro`。
4. 错误文案策略：

   * 与现有 `AttributeError` 风格保持一致，优先使用
     `"'%s' object has no attribute '%s'"` 风格模板。

   * `KlassMro` 版本可用 `klass->name()` 作为类型名来源；
     `InstanceTypeMro` 版本可沿 `PyObject::GetKlass(instance)->name()`。

### 第三步：调用方分层治理（按语义选择 Lookup 或 Get）

1. 保持“允许 miss 分支”的调用点继续使用 `Lookup`，避免误抛异常：

   * 例如 `Runtime_NewStr` 回退 `<object at ...>` 的路径。

   * 比较运算中“无重载则回退默认语义”的路径。
2. 对“miss 必须报错”的调用点，逐步切换到 `Get`，减少重复抛错样板：

   * 优先考虑 `PyTypeObjectKlass::Virtual_GetAttr` 等本就会在 miss 抛 `AttributeError` 的路径。
3. 对暂不切换的路径，保持现状，确保本次变更聚焦并可控。

### 第四步：测试与回归验证

1. 编译验证：

   * 重新生成并构建 `ut` 目标，确保重命名无链接/声明遗漏。
2. 单测验证：

   * 运行现有 `ut` 全量或至少覆盖 objects/runtime/interpreter 相关分组。
3. 行为回归点（重点）：

   * magic method 查找 miss 时仍按原语义处理（TypeError 或回退逻辑）。

   * `getattr` 相关路径在 `is_try=false` 时仍返回 `AttributeError`。
4. 必要时补充最小单测：

   * 新增对 `Runtime_GetPropertyIn*` 的命中/未命中/异常传播三态断言。

## 风险与规避

* 风险 1：误把允许 miss 的路径切到 `Get` 导致行为变“更严格”。

  * 规避：逐调用点按语义审查，先默认保守保持 `Lookup`。

* 风险 2：重命名遗漏导致编译错误或 ODR/链接问题。

  * 规避：全仓符号检索确认旧名字为零。

* 风险 3：`AttributeError` 文案不一致导致测试断言波动。

  * 规避：复用仓库既有错误文案模板。

## 交付验收标准

1. 全仓不再出现 `Runtime_FindPropertyInKlassMro` / `Runtime_FindPropertyInInstanceTypeMro` 旧符号。
2. `Runtime_LookupPropertyInKlassMro` / `Runtime_LookupPropertyInInstanceTypeMro` 可正常编译并被原调用方使用。
3. 新增 `Runtime_GetPropertyInKlassMro` / `Runtime_GetPropertyInInstanceTypeMro`，并通过至少一条调用链验证其 `AttributeError` 语义。
4. 相关测试通过，无新增失败用例。

