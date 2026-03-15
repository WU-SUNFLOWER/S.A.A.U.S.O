# 通用 Descriptor 协议化（实例+类型读路径）三阶段升级计划

## 1. Summary

本计划用于把当前“`PyFunction/NativePyFunction` 特判 + 内建方法 owner 校验”的方案升级为“通用 descriptor 读路径协议”，覆盖：

* 实例属性读取路径（`instance.__getattribute__` 语义主链）；

* 类型对象属性读取路径（`type.__getattribute__` 语义主链）；

* 首批行为对齐 `property` / `classmethod` / `staticmethod`；

* 本轮仅做读路径协议（`__get__` + 优先级），不做 `__delete__`，写路径按现有机制保留。

用户已确认的关键决策：

* 兼容策略：严格对齐 CPython 3.12；

* 覆盖范围：实例 + 类型读路径；

* 推进节奏：分阶段、可随时落地；

* 验收门槛：关键专项 + `BasicInterpreterTest.*` 全量回归；

* 代码规范：遵循 SOLID、Google C++ Style Guide，新增/修改接口与关键逻辑补充必要中文注释（按 `AGENTS.md`）。

***

## 2. Current State Analysis

### 2.1 当前实现形态（已具备的基础）

* 内建方法安装时写入“方法描述符元数据”：

  * `native_call_kind`

  * `descriptor_owner_type`

  * 入口在 [src/builtins/builtins-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-utils.cc)

* 调用阶段有统一 receiver 归一化入口：

  * [src/runtime/runtime-py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-function.cc)

* 调用链接入点已覆盖：

  * [src/interpreter/interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc)

  * [src/interpreter/interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc)

  * [src/objects/py-function-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function-klass.cc)

### 2.2 当前缺口（本计划要解决）

* `Generic_GetAttr` 与 `Generic_GetAttrForCall` 仍以函数类型特判为主，未形成通用 descriptor 绑定协议：

  * [src/objects/py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)

* `PyTypeObjectKlass::Virtual_GetAttr` 仅做 MRO 查找，未做 descriptor 优先级与绑定：

  * [src/objects/py-type-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc)

* 仓库暂无 `property/classmethod/staticmethod` 对象类型与初始化导出。

* 当前测试覆盖“方法描述符”场景较多，但“通用 descriptor 协议”覆盖不足。

***

## 3. Proposed Changes

## Phase A（P0）协议内核：通用 `__get__` 分派与优先级落地

### A.1 新增 Descriptor 运行时门面（单一协议入口）

**新增文件**

* `src/runtime/runtime-descriptor.h`

* `src/runtime/runtime-descriptor.cc`

**职责**

* 提供统一 API（命名可按现有风格微调）：

  * 判断对象是否为 descriptor（是否定义 `__get__`，以及是否为 data descriptor）；

  * 在实例读路径执行 descriptor 优先级决策；

  * 在类型读路径执行 descriptor 绑定决策（`obj=NULL`）。

**设计约束**

* 不在解释器 opcode 侧散落判断，全部收敛到 runtime 门面；

* 与 `Runtime_LookupPropertyIn*` 分工清晰：查找与绑定分离；

* 关键边界（data vs non-data、实例字典覆盖规则）在函数注释中写明“为什么”与不变量。

### A.2 改造实例读路径（`PyObjectKlass::Generic_GetAttr`）

**修改文件**

* `src/objects/py-object-klass.cc`

* （必要时）`src/objects/py-object-klass.h`

**目标行为（对齐 CPython 3.12）**

* data descriptor 优先于实例字典；

* 实例字典优先于 non-data descriptor；

* 统一通过 runtime-descriptor 门面执行 `__get__`；

* `__getattr__` 仍保持兜底语义。

### A.3 改造调用优化路径（`Generic_GetAttrForCall`）

**修改文件**

* `src/objects/py-object-klass.cc`

* `src/objects/py-object.cc`

**目标行为**

* 把“是否可拆 `(method, self)`”从“类型特判”升级为“协议判定”；

* 在不破坏 call-shape fast path 的前提下，优先保证语义正确；

* 若对象属于非可拆分 descriptor，回落到通用路径，避免错误绑定。

### A.4 改造类型读路径（`PyTypeObjectKlass::Virtual_GetAttr`）

**修改文件**

* `src/objects/py-type-object-klass.cc`

**目标行为**

* 类型属性读取同样走 descriptor 读协议；

* 对 descriptor 的 `__get__(obj, type)` 传参符合 CPython 语义（类型路径 `obj=NULL`）；

* 维持 `is_try` / 抛错路径现有契约不变。

***

## Phase B（P1）首批对象语义：`property/classmethod/staticmethod`

### B.1 新增三类对象与 klass

**新增文件（按现有对象/klass模式）**

* `src/objects/py-property.h`

* `src/objects/py-property.cc`

* `src/objects/py-property-klass.h`

* `src/objects/py-property-klass.cc`

* `src/objects/py-classmethod.h`

* `src/objects/py-classmethod.cc`

* `src/objects/py-classmethod-klass.h`

* `src/objects/py-classmethod-klass.cc`

* `src/objects/py-staticmethod.h`

* `src/objects/py-staticmethod.cc`

* `src/objects/py-staticmethod-klass.h`

* `src/objects/py-staticmethod-klass.cc`

### B.2 接入对象系统与 Isolate/Klass 注册表

**修改文件**

* `src/objects/object-type-list.h`

* `src/execution/isolate-klass-list.h`

* `src/execution/isolate.h`（宏展开字段随列表自动生成时仅做必要对齐）

* `src/objects/object-checkers.h`

* `src/objects/object-checkers.cc`

### B.3 提供类型方法并导出到 builtins

**新增文件**

* `src/builtins/builtins-py-property-methods.h/.cc`

* `src/builtins/builtins-py-classmethod-methods.h/.cc`

* `src/builtins/builtins-py-staticmethod-methods.h/.cc`

**修改文件**

* `src/interpreter/builtin-bootstrapper.cc`（`InstallBuiltinTypes` 导出）

* 相应 `Klass::Initialize`（安装 `__new__/__init__/__get__` 等方法）

### B.4 本轮语义边界（与用户决策一致）

* `property`：先支持 getter 语义（只读）；setter/deleter 留给后续阶段；

* `classmethod`：通过 `__get__` 返回绑定到“owner type”的可调用对象；

* `staticmethod`：通过 `__get__` 返回原函数对象，不注入 receiver。

***

## Phase C（P2）一致性收敛与性能回补

### C.1 统一错误语义与文案

**修改文件（按实际影响点）**

* `src/runtime/runtime-descriptor.cc`

* `src/objects/py-object-klass.cc`

* `src/objects/py-type-object-klass.cc`

* `src/runtime/runtime-py-function.cc`

**目标**

* descriptor 缺参与绑定错误文案统一；

* `is_try` 与异常传播边界一致；

* 减少历史特判残留文案分叉。

### C.2 识别并收敛历史特判

**重点检查**

* `GetAttrForCall` 中仅按 `IsPyFunction/IsNativePyFunction` 的遗留逻辑；

* 内建 `__new__` 的必要特判仅保留协议必需部分，删除冗余分支。

### C.3 性能守护（语义正确后再做）

* 保留现有 call-shape fast path 框架；

* 对 descriptor 协议路径增加最小必要 cache 点（若需要）；

* 任何性能优化不得绕过协议正确性。

***

## 4. Assumptions & Decisions

* 决策：本轮主目标是“读路径协议化”，写路径（`__set__/__delete__`）不纳入本轮实现。

* 决策：首批只要求 `property/classmethod/staticmethod` 在读路径行为严格对齐 CPython 3.12。

* 决策：分阶段落地，每阶段可独立编译、独立回归、独立合入。

* 假设：现有 Accessor 机制（`__class__/__dict__`）可与 descriptor 协议并存，不改变其 MVP 边界。

* 非目标（本轮不做）：

  * `__delete__` 语义与全量 data descriptor 写路径；

  * `__set_name__`；

  * 对所有内建对象一次性补齐完整 descriptor 家族行为。

***

## 5. Verification Steps

### 5.1 编译与静态检查

* `ninja -C out/unittest ut`

* 语言诊断（确保无新增编译错误与静态错误）。

### 5.2 新增/修改单测（关键专项）

**建议新增文件**

* `test/unittests/objects/descriptor-unittest.cc`

* `test/unittests/interpreter/interpreter-descriptor-unittest.cc`

**修改文件**

* `test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc`

* `test/unittests/BUILD.gn`

**覆盖点**

* 实例读路径优先级：data descriptor > instance dict > non-data descriptor；

* 类型读路径绑定：`obj=NULL` 行为正确；

* `property/classmethod/staticmethod` 关键语义；

* `GetAttrForCall` 与 call-shape 下的 receiver 注入正确性；

* 错误路径与文案关键片段对齐。

### 5.3 全量回归

* `.\out\unittest\ut.exe --gtest_filter=BasicInterpreterTest.*`

* 保持现有 receiver 归一化相关测试持续通过（含 `CallBuiltinMethodViaTypeObject*`）。

### 5.4 验收标准

* 语义：上述测试全部通过，且行为与 CPython 3.12 对齐目标一致；

* 架构：descriptor 绑定逻辑可在单一 runtime 门面定位，不再依赖散落特判；

* 可维护：新增/修改接口与关键逻辑具备必要中文注释，满足 `AGENTS.md` 与 Google C++ 风格要求。

