# Accessor 机制收敛 `__class__`/`__dict__` 改造计划

## 1. Summary

目标是在 S.A.A.U.S.O 中引入“早期 V8 风格 Accessor 过渡层”，以统一处理保留属性 `__class__` 与 `__dict__` 的读写语义，收拢当前分散在 Factory、具体类型构造和模块加载中的字典镜像写入逻辑，并为后续完整 Python descriptor 机制提供单一扩展入口。

本计划遵循以下已确认决策：

* 首版接入层：`Generic_GetAttr/Generic_SetAttr`（贴近 CPython 通用属性语义）。

* `obj.__class__ = x`：MVP 严格禁止（TypeError）。

* `obj.__dict__ = x`：MVP 严格禁止（TypeError，按只读处理）。

* 迁移节奏：一步到位删除现有分散 `ST(class)` / `ST(dict)` 写入点。

* 架构约束：符合 SOLID 与 Google C++ Style Guide，保持高可读性、可维护性。

***

## 2. Current State Analysis

### 2.1 分散写入点

* `ST(class)` 写入（共 5 处）：

  * `src/heap/factory.cc` `Factory::NewPythonObject`

  * `src/objects/py-list-klass.cc` `Virtual_NewInstance`

  * `src/objects/py-string-klass.cc` `Virtual_NewInstance`

  * `src/objects/py-tuple-klass.cc` `Virtual_NewInstance`

  * `src/modules/module-loader.cc` `InitializeModuleDict`

* `ST(dict)` 写入（共 3 处）：

  * `src/heap/factory.cc` `Factory::NewPyFunction`

  * `src/heap/factory.cc` `Factory::NewPyModule`

  * `src/heap/factory.cc` `Factory::NewPyTypeObject`

### 2.2 现有属性读写主链路

* `PyObject::GetAttr/SetAttr` 分发到 klass vtable。

* 默认对象语义在 `PyObjectKlass::Generic_GetAttr/Generic_SetAttr`：

  * 读：实例 `properties`（`__dict__`）优先，再 MRO，再 `__getattr__`。

  * 写：直接写实例 `properties`。

* 当前 `__class__`/`__dict__` 依赖实例 dict 镜像，存在：

  * 语义分散，新增路径易漏写。

  * 与“类型元信息驱动语义”的单一真相不一致。

  * 后续 descriptor 演进缺少统一协议入口。

### 2.3 与 CPython 对齐目标

* 首版不追求一次实现完整 descriptor 协议。

* 首先建立“保留名属性由统一机制处理”的入口与优先级，作为 descriptor 前置基础设施。

***

## 3. Proposed Changes

## 3.1 新增 Accessor 过渡层（对象域）

### 文件

* 新增 `src/objects/accessor.h`

* 新增 `src/objects/accessor.cc`

* 修改 `BUILD.gn`（把新增文件纳入 `saauso_core`）

### 设计与接口

* 定义轻量描述结构（函数指针风格，贴近 V8 早期概念）：

  * Getter：`MaybeHandle<PyObject> (*)(Handle<PyObject> receiver)`

  * Setter：`MaybeHandle<PyObject> (*)(Handle<PyObject> receiver, Handle<PyObject> value)`

* 定义统一查询入口（仅处理“是否命中保留名 + 执行访问器”）：

  * `Maybe<bool> Accessor::TryGet(Handle<PyObject> receiver, Handle<PyObject> name, Handle<PyObject>& out_value)`

  * `Maybe<bool> Accessor::TrySet(Handle<PyObject> receiver, Handle<PyObject> name, Handle<PyObject> value)`

* 返回约定：

  * `Just(true)`：命中 Accessor 且处理完成（成功返回值或已抛异常并返回空 MaybeHandle）。

  * `Just(false)`：未命中 Accessor，调用方继续走原有链路。

  * `Nothing`：Accessor 内部异常路径，调用方直接传播。

### MVP 注册项

* `__class__`

  * Get：返回 `PyObject::GetKlass(receiver)->type_object()`。

  * Set：抛 `TypeError`（只读，禁止赋值）。

* `__dict__`

  * Get：若对象有 `properties` 则返回该 dict；否则按现有风格抛 `AttributeError`。

  * Set：抛 `TypeError`（只读，禁止赋值）。

## 3.2 接入 Generic 属性读写入口

### 文件

* 修改 `src/objects/py-object-klass.cc`

* （如需声明依赖）修改 `src/objects/py-object-klass.h`

### 改造点

* `Generic_GetAttr`：

  * 在“实例 dict 查找”之前，先调用 `Accessor::TryGet`。

  * 若命中则直接返回，未命中再走原流程（实例 dict -> MRO -> `__getattr__`）。

* `Generic_SetAttr`：

  * 在属性名类型校验通过后、实例 dict 空判断前，先调用 `Accessor::TrySet`。

  * 若命中则直接返回，未命中再走原流程（普通属性写入 `properties`）。

### 预期收益

* 使 `__class__` / `__dict__` 拥有固定且集中语义，不再依赖镜像键值。

* 为 descriptor 优先级扩展预留统一插口（未来可把“保留名表”扩展为“描述符决策层”）。

## 3.3 一步到位删除分散镜像写入

### 文件与删除项

* `src/heap/factory.cc`

  * 删除 `Factory::NewPythonObject` 中 `ST(class)` 写入。

  * 删除 `Factory::NewPyFunction/NewPyModule/NewPyTypeObject` 中 `ST(dict)` 写入。

* `src/objects/py-list-klass.cc`

  * 删除 `Virtual_NewInstance` 中非 exact 分支 `ST(class)` 写入。

* `src/objects/py-string-klass.cc`

  * 删除 `Virtual_NewInstance` 中非 exact 分支 `ST(class)` 写入。

* `src/objects/py-tuple-klass.cc`

  * 删除 `Virtual_NewInstance` 中非 exact 分支 `ST(class)` 写入。

* `src/modules/module-loader.cc`

  * 删除 `InitializeModuleDict` 中 `ST(class)` 写入。

### 注意点

* 仅删除镜像写入，不改变对象真实类型绑定逻辑（`mark_word -> Klass`）与 `properties` 生命周期管理。

* 确保模块、函数、类型对象仍按既有逻辑分配/持有 `properties`，避免引入空指针访问。

## 3.4 测试补强与回归覆盖

### 文件

* 修改 `test/unittests/objects/attribute-unittest.cc`

* 修改 `test/unittests/objects/py-module-unittest.cc`

* （如需要新增文件）修改 `test/unittests/BUILD.gn`

### 新增/调整测试点

* `obj.__class__` 读取返回真实 type object（不依赖实例 dict）。

* 给实例 dict 人工注入同名键 `__class__`，读取仍返回 Accessor 结果（验证优先级）。

* `obj.__class__ = x` 抛 `TypeError`。

* `obj.__dict__` 读取返回 `properties`。

* `obj.__dict__ = x` 抛 `TypeError`。

* 模块对象回归：不再要求 `module.__dict__` 内含 `__class__`/`__dict__` 镜像键，但 `GetAttr("__dict__")` 语义保持正确。

***

## 4. Assumptions & Decisions

* 决策：首版 Accessor 仅覆盖实例对象通用路径（`PyObjectKlass::Generic_*`），不扩展到 `PyTypeObjectKlass::Virtual_GetAttr/SetAttr`。

* 决策：`__class__`、`__dict__` 在 MVP 作为只读保留属性处理，先保证语义一致性与可维护性。

* 决策：分散镜像写入在同一改造中删除，不保留双轨。

* 假设：现有代码路径中不存在对“`properties` 字典内必须物理包含 `__dict__`/`__class__` 键”的强依赖；若 UT 暴露此依赖，将以行为测试为准做最小修正。

* 非目标（本次不做）：

  * 完整 data descriptor / non-data descriptor 优先级矩阵。

  * `__class__` 受限可写（兼容布局校验）。

  * `type` 对象属性系统统一纳入 Accessor。

***

## 5. Verification Steps

1. 编译验证

* 执行 `gn gen` 与 `ninja`（按仓库现有方式）确保核心库与测试可编译。

1. 单测验证（至少）

* 运行对象属性相关 UT：

  * `attribute-unittest`

  * `py-module-unittest`

  * `py-object-unittest`

* 如仓库支持整组执行，运行 `ut` 全量回归。

1. 行为验收

* 验证 `__class__` 读取正确、写入报错。

* 验证 `__dict__` 读取正确、写入报错。

* 验证普通属性读写流程不回归（`SetAttr/GetAttr` 正常）。

* 验证模块加载路径无回归（导入与模块属性初始化通过）。

1. 结构验收

* 全仓检索确认已无分散 `PyDict::Put(..., ST(class)...)` / `PyDict::Put(..., ST(dict)...)` 镜像写入残留（除 Accessor 注册或专门测试构造外）。

