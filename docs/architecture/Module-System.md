# 模块系统（Import System）

本文档收口 S.A.A.U.S.O 的模块系统（import）架构与语义要点，覆盖职责拆分、查找规则与关键实现索引。

## 1. 分层职责（`src/modules`）

按“语义编排/定位/加载/状态”拆分：

- `ModuleManager`：模块系统门面与状态持有者，维护 `sys.modules`（dict）与 `sys.path`（list），并向解释器暴露统一入口 `ImportModule(...)`（见 [module-manager.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.h)）。
- `ModuleImporter`：导入语义编排层，负责 dotted-name 分段导入、`fromlist` 语义、父子模块绑定，以及在 `level` 非 0 时配合相对导入名解析（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
- `ModuleNameResolver`：相对导入名解析器，基于 `globals["__package__"]` / `globals["__name__"]` 与是否为 package（存在 `globals["__path__"]`）决定相对导入基准，并处理越界/无父包等错误（见 [module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)）。
- `ModuleFinder`：纯文件系统定位层，只负责在给定 search path 中查找 module/package 的位置，不执行模块体（见 [module-finder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-finder.cc)）。
- `ModuleLoader`：加载与执行层，负责创建 `PyModule`、初始化 `__name__/__package__/__file__/__path__` 等关键字段，并执行模块体（source 或 pyc）；同时负责 builtin 模块的创建（见 [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)）。
- `BuiltinModuleRegistry`：内建模块注册表，仅维护 name → init 函数映射，供 `ModuleLoader` 优先命中（见 [builtin-module-registry.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-registry.cc)）。
- `ModuleUtils`：模块系统通用工具（模块名合法性、package 判定、`__path__` 提取等），用于减少 ModuleImporter/Loader 的重复样板代码（见 [module-utils.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.h)）。

## 2. sys 状态初始化

- `sys.modules` 初始为空 dict；`sys.path` 初始为 `['.']`（见 [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)）。

## 3. 查找与加载规则（文件系统）

- 文件系统查找优先级：对每个 base path，先尝试 package（`<base>/<name>/__init__.py` → `<base>/<name>/__init__.pyc`），再尝试 module（`<base>/<name>.py` → `<base>/<name>.pyc`），并遵循“`.py` 优先、`.pyc` 兜底”（见 [module-finder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-finder.cc)）。
- 不默认扫描 `__pycache__/<name>.*.pyc` 这类 tagged 缓存路径；embedder 若只分发字节码，建议直接提供同目录的 `<name>.pyc` 或 `<pkg>/__init__.pyc`。
- 搜索路径来源：导入顶层段使用 `sys.path`；导入子模块使用 `package.__path__`，若父模块不是 package 则 fail-fast（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。

## 4. 关键语义点（以仓库现状为准）

- 父子模块绑定：导入 `pkg.sub` 后会把子模块写入父模块 dict（`pkg.sub = <module>`），保证 Python 层可见（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
- `fromlist` 决定 `IMPORT_NAME` 返回值：当 `fromlist` 为空且导入 dotted-name 时返回顶层包，否则返回最后导入的模块对象（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
- 循环导入：CPython 语义通常要求“执行模块体前就将 module 放入 sys.modules”。当前实现把模块写入 `sys.modules` 的时机在执行模块体之后，因此循环导入行为不保证对齐 CPython，后续需要专门补齐（见 [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)）。
- 内建模块（builtin）：
  - 内建模块通过注册表机制优先命中（见 [builtin-module.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module.h)）。当前内建模块列表含 `sys/math/random/time`；`sys.modules/sys.path` 指向 `ModuleManager` 持有对象（见 [builtin-sys-module.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-sys-module.cc)）。

