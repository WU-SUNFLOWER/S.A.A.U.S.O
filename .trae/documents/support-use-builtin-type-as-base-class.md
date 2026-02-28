## 目标与范围

在当前 SAAUSO 的 “Klass–Object 二元架构” 下，渐进式实现 “用户自定义 Python 类可继承内建容器（list/dict/tuple/str…）并保持正确的对象布局、GC 扫描与内建方法可用”。

本次重构以可在一个星期内交付为约束，按阶段确保每一步都能编译通过并有可验证的行为改进。

### 最小可交付语义（MVP）

* `class C(list): ...` 可创建实例 `c = C()`，且：

  * 实例在物理布局上是 list-like（可被 list 的 slot/GC 扫描安全处理）。

  * 内建 list 方法（至少 append/pop/len/iter/subscr/contains）对 `C()` 实例可用，不因 cast/assert 失败崩溃。

  * `c.x = 1` 这类实例属性可用（子类具备 __dict__ 语义），不影响 list 本体仍无 __dict__。

* 多继承约束：同一个 class 的 bases 中最多允许一个“带特殊布局的内建类型”（single native base）。否则创建 class 时抛出 TypeError。

### 非目标（阶段性明确不做）

* 不追求完全对齐 CPython 对 “heap type slot filling / 特殊方法覆盖影响 slot 行为” 的全部语义。

* 不保证 `__len__` 等 magic method 的 Python 层覆盖一定能影响 VM 内部的 slot（可作为后续阶段增强）。

## 总体策略（核心设计）

引入“Native Layout（原生布局）”概念，将 “Python 继承（MRO）” 与 “对象物理布局/slot/GC 扫描” 在 Klass 层打通：

* 对每个 Klass 标记其 native layout kind（例如：kPyObject / kList / kDict / kTuple / kString…）。

* 对用户自定义 Klass 记录其 native layout base（即主基类的 own\_klass），并在 class 创建时做合法性检查（single native base）。

* 派生类实例分配时按 native layout base 的物理布局分配并初始化底层字段，同时将对象的 klass 设为派生 klass，从而保证：

### NativeLayoutKind 驱动的对象分配基建 API

目标：将 “按 native 物理布局分配并建立对象不变量（layout invariants）” 与 “Python 语义编排（签名解析/默认初始化/调用 `__init__`）” 解耦，避免在 `*Klass::Virtual_ConstructInstance` 中复制并分叉初始化协议。

分层原则：

* 分配基建 API（窄职责）只负责：

  * 基于 `native_layout_kind`（以及变长对象的 layout 参数）选择对象大小与字段集合。

  * 分配对象并初始化必须字段（如 list 的 `length_/array_`，dict 的内部表指针与计数，tuple/str 的 inline payload 元信息等）。

  * 将对象的 klass 设为传入的 `klass_self`（允许 exact builtin 与派生 klass）。

  * 按策略初始化实例属性存储（exact builtin 可维持 `properties_ = null`；subclass 分配 `properties_ = PyDict::NewInstance()`）。

* `*Klass::Virtual_ConstructInstance` 只负责语义编排：

  * exact builtin 的参数约束与默认初始化逻辑（例如 `list(iterable)`、`dict(mapping, **kw)` 等）。

  * subclass：若定义了 `__init__` 则分配后调用之；否则回落到 base 的默认初始化语义。

接口形态（建议）：

* 固定大小容器（list/dict）：

  * `AllocateListLike(Tagged<Klass> klass, int64_t init_capacity, PropertiesPolicy policy)`

  * `AllocateDictLike(Tagged<Klass> klass, PropertiesPolicy policy)`

* 变长/inline payload（tuple/str）：

  * `AllocateTupleLike(Tagged<Klass> klass, int64_t length, PropertiesPolicy policy)`

  * `AllocateStringLike(Tagged<Klass> klass, int64_t length_bytes, PropertiesPolicy policy, ...)`

说明：

* tuple/str 的数据 inline 在对象中，因此分配必须携带长度/字节数等 layout 参数（等价于 CPython 的 `tp_basicsize/tp_itemsize`、V8 的 instance\_size 计算）。

* `native_layout_kind` 在语义上对标 V8 的 `instance_type`，用于 fast path guard 与 GC visitor 分派；分配 API 则对标 “基于 instance type/size 的对象分配入口”。

### 业界对比与路线校准（V8 / CPython）

为了验证该方案是否符合业界主流路线，这里将 SAAUSO 当前的概念与 V8/CPython 做对照，并据此校准本重构的关键落点。

#### 与 V8（Map → InstanceType/VisitorId）的对应关系

* V8：对象头指向 Map；Map 中编码 instance\_type、instance\_size、elements\_kind、in-object properties 形态，并通过 visitor id 指向 GC 扫描策略。

* SAAUSO：对象头（MarkWord）可得到 Klass；Klass 提供 instance\_size/iterate（GC 扫描）与一组 slot（vtable）用于语义实现。

本方案的 `native_layout_kind` 在语义上等价于 V8 的 `instance_type`；它解决的是“快速判定对象的物理布局类别”，避免把非 list 布局当成 list 访问字段。由此：

* 类型判定：从 “klass 指针相等（exact type）” 扩展为 “layout kind 相等（layout-compatible）”，对应 V8 的 instance\_type 检测。

* GC 扫描：由 layout kind 决定 iterate 的字段集合，等价于 V8 的 visitor 分派。

* 多继承约束：只允许 single native base，本质上是避免一个对象同时声称自己拥有两种 incompatible instance\_type；这与 V8 里不同 instance\_type 的对象不能共享同一个物理布局的原则一致。

校准结论：

* 需要将 `native_layout_kind` 实现成“低成本、可用于 hot path 的紧凑标签”（例如C++小整数/枚举），并让所有 list-like/dict-like 的 fast path 仅依赖该标签做 guard。

* `native_layout_base` 更接近 “layout 归属的 Map 家族”，用于构造与 slot 复用时的分派；它不是必须，但能让实现更直接（从 base 复用构造/扫描/部分 slot）。

#### 与 CPython（PyTypeObject → tp\_basicsize/tp\_traverse/tp\_new）的对应关系

* CPython：每个类型有 `tp_basicsize/tp_itemsize` 描述对象布局，`tp_traverse` 描述 GC 扫描，`tp_new/tp_init` 描述构造；heap type（用户子类）在创建时会基于 base type 计算布局并填充 slot。

* 本方案：将 “布局类别与扫描策略” 下沉到 Klass（类似 tp\_basicsize/tp\_traverse），并在 class 创建时确定 single native base，从而让派生实例按 base 布局分配并可复用 base 的 slot。

校准结论：

* 阶段性不实现完整的 “slot filling（magic method 覆盖影响 slot）” 是可接受的，但必须在计划中明确，并将其作为可选阶段增强（对应 CPython heap type 的 slot 更新）。

  * `type(x)` 语义由对象的 klass 决定

1. 盘点内建容器的：

   * construct\_instance / instance\_size / iterate 等 slot

   * builtin methods 中的 `Handle<PyXXX>::cast(self)` 热点位置
2. 盘点 class 创建路径（build\_class/type 相关）与 MRO 计算路径，确认 native base 决策点放置位置。
3. 为后续行为验证准备最小测试用例集合（list 子类、dict 子类、GC 触发）。

验收标准：

* 不改功能，仅形成“需要改哪些文件/函数”的清单与测试脚本列表。

## 阶段 1：为 Klass 引入 Native Layout 元信息（最小侵入）

实现步骤：

1. 在 `Klass` 增加以下字段与访问接口：

   * `native_layout_kind`（枚举或 small int；对标 V8 的 instance\_type，应放在 hot path 可快速读取的位置）

   * `native_layout_base`（Tagged<Klass>，对普通 builtin klass 指向自身；对用户 klass 指向其主基类 builtin klass；对纯对象布局指向 PyObjectKlass）
2. 更新 `Klass::Iterate`，确保新增 Tagged 字段可被 GC root 扫描访问（即使当前 metaspace 不移动，也保持一致性）。
3. 在各 builtin container klass 初始化时设置 `native_layout_kind/base`：

验收标准：

* 编译通过。

* 现有用例不回归。

* 能在调试输出/断言中区分 “PyListKlass vs RawPythonKlass（list base）” 的 layout 信息。

## 阶段 2：在 class 创建时确定并校验 native base（single native base）

实现步骤：

1. 在创建 Python class（Runtime\_CreatePythonClass 或 build\_class 调用链）中扫描 bases（supers）：

   * 对每个 base type object 取 `base->own_klass()`，读取其 `native_layout_kind`

   * 选出唯一一个 `native_layout_kind != kPyObject` 的 base 作为主基类（native base）

   * 如果出现两个及以上不同 kind 的 native base：抛 TypeError（多继承冲突）
2. 为新建的 raw python klass 设置：

   * `native_layout_kind = 主基类 kind`

   * `native_layout_base = 主基类 own_klass`
3. 保持 supers/mro 的现有行为不变（仍用于属性查找与 isinstance）。

验收标准：

* `class C(list, dict): pass` 在类创建阶段抛 TypeError。

* `class C(list): pass` 类创建成功，且其 klass 能报告 native\_layout\_kind=kList。

## 阶段 3：让 list 的 slot/builtin 支持 “list-like 对象”（支持子类的核心）

关键原则：

* “能进入 list 的 slot/builtin 的对象”必须在物理布局上确实是 PyList（或兼容布局），否则严格 fail-fast。

实现步骤（按改动面从小到大）：

1. 新增统一的 list-like 判定与布局 cast 工具（避免滥改全局 cast 语义）：

   * `IsListLike(object)`：判断 `PyObject::GetKlass(object)->native_layout_kind == kList`

   * `CastListLike(object)`：在 IsListLike 前提下返回 `Handle<PyList>` / `Tagged<PyList>`
2. 修改 `PyListKlass` 的以下 slot，使其面向 list-like 而非 exact list：

   * `Virtual_InstanceSize`：对 list-like 返回 `sizeof(PyList)` 对齐后的大小

   * `Virtual_Iterate`：对 list-like 扫描 `array_`

   * 其他内部使用 `Handle<PyList>::cast(self)` 的 slot：改用 `CastListLike(self)`
3. 重写/调整 `PyListKlass::Virtual_ConstructInstance` 以支持 `klass_self` 为派生 klass：

   * 允许 `klass_self` 的 native\_layout\_kind==kList

   * 分配一个 PyList 物理对象并初始化 `length_/array_`

   * 将对象的 klass 设为 `klass_self`（而不是 PyListKlass::GetInstance）

   * 对 “exact list” 仍保持 `properties_ = null`（无 __dict__）

   * 对 “list 子类” 初始化 `properties_ = PyDict::NewInstance()`（具备实例属性）
4. 修改 list 的 builtin methods（builtins-py-list-methods）中所有 `Handle<PyList>::cast(self)`，统一替换为 list-like cast。

验收标准（行为用例）：

* Python 侧：

  * `class C(list): pass; c=C(); c.append(1); len(c)==1; c.pop()==1`

  * `c.x=1; c.x==1`

* 不发生 assert/cast 崩溃。

* GC 扫描路径不出现越界/漏扫（通过构造大量 list 子类实例并触发 GC 的回归用例验证）。

## 阶段 4：把 native layout 机制推广到 dict/tuple/str（逐类推进）

推进顺序建议：

1. dict（最容易暴露 GC/布局问题，且方法多依赖 cast）
2. tuple（不可变但有 elements 扫描）
3. str（不可变，可能牵涉 intern / buffer 管理）

对每种类型重复“阶段 3 的套路”：

* 增加 `IsXLike/CastXLike`

* 让 `*Klass` 的 instance\_size/iterate/construct\_instance 支持 `klass_self` 为派生 klass

* 修改对应 builtins 的 cast 逻辑

* 为子类是否具备 __dict__ 做一致策略：base type 维持现状，子类初始化 properties dict

验收标准：

* `class C(dict): ...` 的基本 set/get/len/iter 可用

* `class C(tuple): ...` 可构造且元素扫描正确

* `class C(str): ...` 至少可构造/len/索引可用（具体按现有 str 特性覆盖）

## 阶段 5（可选增强）：支持 “magic method 覆盖影响 slot”（半对齐 CPython）

问题点：

* 当前 slot 直接走 vtable，派生类若想覆盖 `__len__` 等，必须影响 vtable 行为。

实现思路（可控复杂度版本）：

1. 在 class 创建完成后，对特定 magic method 列表（如 `__len__`, `__iter__`, `__contains__`, `__getitem__`）进行一次性检查：

   * 若派生类在自身 klass\_properties 中定义了该方法，则将对应 vtable slot 指向一个通用 wrapper：

     * wrapper 先按 MRO 调用该 magic method

     * 若未定义，则 fallback 到 native base slot
2. 仅覆盖少量最常用操作，避免全量 slot filling 的工程量。

验收标准：

* `class C(list): def __len__(self): return 123` 时 `len(C())==123`（在启用该增强的前提下）。

## 风险与回滚策略

* 风险：把 list/dict slot 放宽到 list-like/dict-like 会影响现有 exact 类型假设。

  * 缓解：list-like 判定必须基于 klass 的 native\_layout\_kind；对不满足者立即抛 TypeError 或断言 fail-fast，禁止“误把非 list 布局当 list”。

* 风险：子类 __dict__ 与 base 无 __dict__ 的语义差异。

  * 缓解：严格区分 exact builtin vs subclass：exact builtin 保持 properties\_=null；subclass 分配 properties dict。

* 回滚：每一类容器的支持可通过单独提交/阶段性合入控制；出现问题可仅回退某一类型的支持，不影响其他类型。

## 交付检查清单（每阶段完成后）

* 编译通过（Windows/Linux）且无新增警告

* 解释器/单测通过

* 新增用例覆盖：list/dict 子类 + 实例属性 + GC 触发

## 旧 API（IsPyList/IsPyDict/IsPyString…）的处置与演进策略

### 是否能“完全移除”？

不建议在本次一个月窗口内以“完全移除”为目标，原因是：

* 当前 `IsPyList/IsPyDict/...` 语义是 “exact type（klass 指针相等）”，大量现有代码以此作为强 guard；直接删除会迫使全仓库改写调用点，风险与回归面过大。

* 支持内建容器子类化后，工程上通常需要同时保留两种判定：

  * “layout-compatible / subtype-friendly”（用于 builtin 方法、GC 扫描、fast path）

  * “exact builtin type”（用于需要保持 base 语义差异的地方，例如 base list 无 __dict__、某些只对 exact builtin 开放的优化）
    这与业界习惯一致：例如 CPython 有 `PyList_Check` 与 `PyList_CheckExact`；V8 既有 instance\_type 检测也有对特定 Map/hidden class 的精确匹配。

### 本计划内的推荐做法（低风险迁移）

1. 保留现有 `IsPyList/IsPyDict/IsPyString...` 的实现与语义，视作 `*Exact`。
2. 新增并推广一组 “XLike” 判定与布局 cast：

   * `IsListLike/IsDictLike/IsStringLike...`（基于 native\_layout\_kind/instance\_type）

   * 对应 `CastListLike/CastDictLike...`
3. 逐类推进时，只在需要支持子类的路径替换判定与 cast：

   * builtin methods

   * vtable slot（instance\_size/iterate/construct\_instance 等）
4. 等迁移完成且回归稳定后，再评估是否要：

   * 将 `IsPyList` 的语义升级为 “list-like”，并新增 `IsPyListExact` 作为旧语义（需要全仓库审计）

   * 或者保持现状，长期并存两套 API（更稳健）

