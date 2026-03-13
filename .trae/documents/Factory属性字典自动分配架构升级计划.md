# Factory 属性字典自动分配架构升级计划

## 1. 结论与方向

你的观察是合理的：当前 `Factory::*Like` 依赖调用方手工传 `allocate_properties_dict`，确实把“实例是否应具备可写实例属性”这一语义泄露给了上层调用者，带来额外心智负担和潜在误用风险。  
改造为“由 `Klass` 持有能力标记，Factory 按 `Klass` 自动决策”是正确方向，并且整体上更接近 CPython 的“类型对象承载实例能力配置、实例创建按类型语义执行”的模型。

## 2. 设计原则

1. **语义归位**：是否为实例分配 properties dict 属于“类型能力”，不应由调用点重复决策。
2. **最小侵入**：尽量保持 `native_layout_kind` 的既有语义稳定（纯布局枚举），避免把能力位混入布局值。
3. **兼容优先**：先做“可行为等价”的 API 收敛，再删除旧开关参数，分阶段降低回归风险。
4. **可验证**：每一阶段都有明确单测覆盖与回归入口。

## 3. 方案选型

### 3.1 推荐方案（实施）

在 `Klass` 新增显式布尔标志（例如 `instance_has_properties_dict_`），并提供 getter/setter。  
Factory 在 `NewDictLike/NewPyListLike/NewPyTupleLike/NewRawStringLike/NewStringLike` 中根据 `klass_self` 自动决定是否分配 properties dict，不再接受外部布尔参数。

### 3.2 不推荐方案（仅评估）

复用 `native_layout_kind` 的最高位承载能力标记。该方案会让“布局枚举”混入“实例能力”，导致：

- `native_layout_kind` 的比较/判等语义变脏；
- `IsPy*` like 判定、建类布局冲突判定、后续扩展位分配复杂化；
- 可读性与调试可观测性下降。

因此本次改造不采用“位复用”。

## 4. 分步实施计划

### Step A：Klass 能力位建模

1. 在 `src/objects/klass.h` 为 `Klass` 增加实例属性字典能力字段与访问器。  
2. 在 `src/heap/factory.cc` 的 `Factory::NewPythonKlass` 中初始化默认值（建议默认 `false`，由具体类型显式开启）。  
3. 梳理并设定能力位赋值点：
   - 内建可子类化布局类型（`list/dict/tuple/str`）在对应 `PreInitialize` 设为 `true`；
   - 普通 `object` 及无需实例属性字典的类型维持 `false`；
   - 动态类（`Runtime_CreatePythonClass`）按 `native_layout_kind` 规则推导后同步设置（特殊布局默认 `true`，`kPyObject` 默认 `true` 或按现有语义校准，以下一步回归结果定稿）。

### Step B：Factory API 去开关化

1. 修改 `src/heap/factory.h`：移除 `*Like` 系列 API 的 `allocate_properties_dict` 参数。  
2. 修改 `src/heap/factory.cc`：在各 `*Like` 实现中统一使用 `klass_self->instance_has_properties_dict()` 判断是否分配。  
3. 保持现有“先置 null，再按需分配”的 GC 安全初始化顺序不变。

### Step C：调用点收敛

1. 修改 `py-list-klass.cc / py-dict-klass.cc / py-tuple-klass.cc / py-string-klass.cc` 的 `Virtual_NewInstance`，删除 `!is_exact_xxx` 传参逻辑。  
2. 保留 `is_exact_xxx` 仅用于现有 `ST(class)` 写入等行为（若仍需要）。  
3. 全仓替换并清理旧参数调用，确保无残留编译入口。

### Step D：语义校准（对齐 CPython 方向）

1. 明确并固定两条语义：
   - “是否可挂实例属性”由类型能力位决定；
   - “是否写入 `ST(class)` 绑定信息”与是否 exact 的现有逻辑保持解耦。  
2. 对照 CPython3.12 的可子类化内建（list/dict/tuple/str）行为，验证子类实例可持有实例属性、exact 内建实例保持现有行为预期。

### Step E：验证与回归

1. 编译验证：全量构建（Windows 当前工具链）。  
2. 单测验证：
   - `test/unittests/objects` 下与 list/dict/tuple/string/new-object 相关用例；
   - 补充/新增“子类实例是否拥有 properties dict”的定向单测。  
3. Python 脚本回归：
   - 现有 `test/python312` 中与 `build_class`、内建容器构造和属性访问相关脚本；
   - 增加最小回归样例：`class MyList(list): pass; x=MyList(); x.a=1` 等。
4. 语义回归门槛：不得破坏已有 `IsPy*Like` 判定、MRO 属性查找、vtable 初始化路径。

## 5. 预期产出

1. `Factory` 对“是否分配 properties dict”实现自动化与单一真源（`Klass`）。  
2. 删除手工布尔开关后，调用 API 更简洁，误用面收敛。  
3. 架构上更接近 CPython 的“类型驱动实例能力”模型，同时保持 S.A.A.U.S.O 当前对象系统约束。

## 6. 风险与应对

1. **风险**：动态类默认能力位设置不当导致行为回归。  
   **应对**：以现有 python312 回归脚本 + 新增子类属性用例双重校验。  
2. **风险**：API 签名变更造成编译点遗漏。  
   **应对**：全仓检索 `allocate_properties_dict` 与 `New*Like(` 调用签名，清零残留。  
3. **风险**：exact 与 subclass 分支行为耦合被误改。  
   **应对**：保留 `ST(class)` 相关断言测试，确保只迁移“properties dict 分配决策”。
