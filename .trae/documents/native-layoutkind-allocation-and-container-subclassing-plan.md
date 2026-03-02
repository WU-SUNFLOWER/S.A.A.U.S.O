## 背景与目标

在现有 “Klass–Object 二元架构” 与已引入的 `NativeLayoutKind/native_layout_base` 基础上，进一步补齐一层“以 native 物理布局驱动的对象分配/初始化基建 API”，用于：

1. 降低 builtin container（list/dict/tuple/str）在“exact vs subclass”构造路径上的逻辑耦合与初始化协议重复。
2. 让 list/dict 的子类化实现可持续扩展，并为后续 tuple/str（变长/inline payload）预留正确的抽象形态。
3. 在此基础上推进 dict 容器的子类化改造，并用单测验证该基建 API 的普适性。

## 范围

本轮实现与验证范围：

1. 更新重构文档：固化“NativeLayoutKind 驱动分配 API”的设计、职责边界与迁移策略。
2. 将 PyList/PyListKlass 里“heap allocate + 字段初始化 + properties 策略”的逻辑抽离为可复用的基建原语；Klass slot 层只保留“语义编排”（签名/extend/是否调用自定义 `__init__`）。
3. 推进 PyDict/PyDictKlass：完成 dict-like 判定与 cast、slot/builtin 迁移、构造分配走同一套基建原语，并新增回归用例验证。

明确不在本轮完成：

* tuple/str 的子类化全面落地（但分配 API 需要在设计层面明确支持变长/inline payload）。

* 完整对齐 CPython heap type 的 slot filling（magic method 覆盖影响 slot）机制。

## 核心设计：NativeLayoutKind 驱动分配 API

### 设计动机

当前 list 子类化为了同时满足：

* list-like 物理布局分配（字段/GC 扫描一致）

* 对象 klass 必须为派生 klass（影响 `type(x)` / MRO 属性查找）

* exact builtin 与 subclass 的 properties 策略差异（base 无 `__dict__`，subclass 有）

导致 `*Klass::Virtual_ConstructInstance` 中出现“语义 + 分配初始化细节”耦合，并绕过 `PyList::NewInstance` 手写初始化协议。为防止该模式在 dict/tuple/str 复制扩散，需要在 objects/heap 层引入可复用的“按 native layout 分配并建立不变量”的原语。

### API 形态（建议）

定义一组 **窄职责** 的分配/初始化入口（名称以现有代码风格为准）：

* 固定大小容器（list/dict）：

  * `NewPyListLike(Tagged<Klass> klass, int64_t init_capacity, PropertiesPolicy policy) -> Handle<PyList>`

  * `AllocateDictLike(Tagged<Klass> klass, PropertiesPolicy policy) -> Handle<PyDict>`

* 变长/inline payload（tuple/str）只在设计中固化接口形态：

  * `AllocateTupleLike(Tagged<Klass> klass, int64_t length, PropertiesPolicy policy) -> Handle<PyTuple>`

  * `AllocateStringLike(Tagged<Klass> klass, int64_t length_bytes, PropertiesPolicy policy, ...) -> Handle<PyString>`

其中：

* `klass` 由 slot 层传入（exact builtin 或派生 klass）。

* `PropertiesPolicy` 用于统一表达：

  * `kNoProperties`（exact builtin 维持现状）

  * `kWithDict`（subclass 具备实例属性）
    -（如需）`kWithDictAndClassBinding`（当前 list 子类写入 `__class__` 的做法可由策略控制是否保留）

### 职责边界

分配 API **只负责**：

* 基于 `NativeLayoutKind`（以及 tuple/str 的长度/bytes 等 layout 参数）选择正确的对象大小与字段集合。

* 分配对象并初始化必须字段（list: `length_/array_`；dict: table/used 等；tuple/str: inline payload/length 等）。

* 设置对象的 klass 为传入的 `klass`。

* 按 `PropertiesPolicy` 初始化 `properties_`（exact builtin 可为 null；subclass 分配 `PyDict`）。

分配 API **不负责**：

* 参数解析（例如 `list(iterable)` 的 iterable 处理）。

* 查找/调用 `__init__`、`__new__` 或语义兼容逻辑。

* 任何可能触发用户代码执行的行为。

### slot 层责任（construct\_instance）

各 builtin container 的 `Virtual_ConstructInstance` 只负责语义编排：

* exact builtin 的签名检查与默认初始化路径（如 `list()` / `list(iterable)`）。

* subclass：

  * 若定义了 `__init__`：分配后调用之（允许位置/关键字参数）。

  * 若未定义：回落到 base 的默认初始化语义（保持与现有实现一致）。

## 实施计划（按步骤）

### 1. 更新重构文档（固化方案）

修改文件：

* `e:\MyProject\S.A.A.U.S.O\.trae\documents\support-use-builtin-type-as-base-class.md`

更新内容：

* 在“总体策略/核心设计”中新增章节：

  * “NativeLayoutKind 驱动的对象分配基建 API”

  * “固定大小 vs 变长/inline payload（tuple/str）接口形态”

  * “职责边界：分配不变量 vs 语义编排”

  * “迁移策略：先 list，再 dict，再 tuple/str；保留 exact/like 判定两套 API”

* 将阶段 3（list）与阶段 4（dict/tuple/str）补充为：先引入分配 API，再逐类迁移 `construct_instance/iterate/instance_size/builtin cast`。

验收：

* 文档中明确 API 的输入/输出/职责边界，并解释为何 tuple/str 需要 layout 参数。

### 2. 引入并落地“list-like 分配原语”，解耦 PyListKlass

目标：

* `PyList::NewInstance` 与 `PyListKlass::Virtual_ConstructInstance` 不再直接手写 heap allocate 初始化协议。

* list-like 分配集中到一个可复用 helper（可放在 `py-list.{h,cc}` 或更底层模块）。

计划改动点（不限定具体函数名，以现有风格为准）：

* 新增 `PropertiesPolicy`（若已有类似枚举则复用）。

* 在 `PyList` 提供：

  * `NewPyListLike(Tagged<Klass> klass_self, int64_t init_capacity, PropertiesPolicy policy)`

  * 让 `PyList::NewInstance` 变成对 `NewPyListLike(PyListKlass::GetInstance(), ...)` 的包装。

* 在 `PyListKlass::Virtual_ConstructInstance`：

  * 只做：exact list 签名检查、默认初始化（extend iterable）、自定义 `__init__` 查找与调用的语义编排。

  * 分配统一调用 `PyList::NewPyListLike(klass_self, ...)`。

  * 保持现有行为：exact list 无 `properties_`；subclass 有 `properties_`。

验收：

* 现有 list 子类测试、sysgc 测试、自定义 `__init__` 测试均通过。

* `Virtual_ConstructInstance` 的“分配初始化细节”显著收敛（仅保留必要调用）。

### 3. 推进 dict：引入 dict-like 判定与 cast、迁移 slot/builtin、接入分配原语

目标：

* `class C(dict): ...` 实例按 dict 物理布局分配，GC traverse 安全，builtin dict 方法对 dict-like 可用。

* 使用同一套“NativeLayoutKind 驱动分配”思路验证普适性。

计划改动点：

* `PyDict`：

  * 新增 `IsDictLike/CastDictLike`（基于 `native_layout_kind == kDict`）。

  * 新增 `AllocateDictLike(Tagged<Klass> klass_self, PropertiesPolicy policy)`。

  * 让 `PyDict::NewInstance` 走 `AllocateDictLike(PyDictKlass::GetInstance(), ...)`。

* `PyDictKlass`：

  * 初始化时设置 `native_layout_kind/base`（在 PreInitialize 阶段）。

  * slot 修改为 dict-like（iterate/instance\_size/len/subscr 等内部 cast 改为 `CastDictLike`）。

  * `Virtual_ConstructInstance` 逻辑参考 list：

    * exact dict 保持现有签名/kwargs 规则与默认初始化语义；

    * subclass：若存在自定义 `__init__` 则调用；否则回落 base 默认语义；

    * 分配统一走 `AllocateDictLike(klass_self, policy)`。

* builtins：

  * `builtins-py-dict-methods.cc` 中所有 `Handle<PyDict>::cast(self)` 改为 dict-like cast。

* runtime/其他路径：

  * 搜索并替换“把 list/dict 当 exact type”的关键路径（truthiness、intrinsics、import 等）里对 dict 的判断/转换，确保 dict 子类不崩溃（按需要逐点迁移）。

验收（行为用例）：

* Python 侧：

  * `class C(dict): pass; c=C(); c['a']=1; len(c)==1; c['a']==1`

  * `c.x=1; c.x==1`（subclass 具备实例属性）

  * `sysgc()` 压力下对象引用不丢失（dict value 与实例属性均可存活）。

### 4. 测试与回归策略

新增/更新测试：

* 新增 dict 子类单测（与 list 形式一致）：

  * 构造、set/get/len、实例属性、sysgc 压力与存活验证。

* 保持并扩展现有 list 子类测试覆盖（确保解耦后不回归）。

运行验证：

* 仅运行相关 suite：`BasicInterpreterTest.*`（确保解释器行为稳定）。

* 运行全量 `ut.exe`（确保无隐藏回归）。

### 5. 风险点与回滚策略

主要风险：

* dict-like 判定放宽后，可能影响既有“exact dict”假设路径；需要在 hot path 处做严格 guard（kind 检查）并在不匹配时抛 TypeError 或 fail-fast。

* `properties_` 策略在 dict/list 上需要保持一致：exact builtin 维持现状（可能为 null），subclass 一律分配 dict，避免半初始化对象被用户代码观察。

回滚策略：

* 分配原语引入可先仅被 list 使用；若 dict 迁移出现回归，可先回退 dict 的迁移，保留 list 的解耦成果。

## 交付产物

* 更新后的重构文档：`support-use-builtin-type-as-base-class.md`

* list 的分配/初始化原语与 slot 解耦

* dict 的子类化支持（slot/builtin/cast/分配原语）与新增单测

* 全量测试通过记录

