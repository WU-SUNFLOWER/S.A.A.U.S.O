# 三步走：在 SAAUSO 全面铺开 MaybeHandle，消除 Handle::null() 的失败二义性

## 0. 结论：你的建议是否有道理？

有道理，而且是更稳妥的落地路径：先把 **runtime** 层的可失败语义彻底类型化（MaybeHandle），再把 **builtins** 层类型化（避免 Python API 暗含“null=失败”），最后再攻克 **objects/vtable/PyObject API** 这一最大依赖面。

同时你提到的要求也成立：必须尽可能用 `src/execution/exception-utils.h` 的 V8-like 宏把调用点写成机械模板，避免 `ToHandle(...)`/`if (...) return ...` 反复散落，降低噪声与误用概率。

## 1. 总体目标（最终形态）

1. **Fallible API（可能抛 Python 异常）**：返回 `MaybeHandle<T>`，失败用 `IsEmpty()` 表达；并且失败时必须已设置 pending exception（error indicator）。

2. **Try/Lookup API（不会抛异常，仅表示缺失）**：继续返回 `Handle<T>` / `Tagged<T>`，`Handle::null()` 仅表示缺失，必须保证不设置 pending exception；并在头文件注释中写清楚“null=缺失，不是错误”。

3. **调用点约束**：凡调用 Fallible API，必须使用 `ASSIGN_RETURN_ON_EXCEPTION(_VALUE)` / `RETURN_ON_EXCEPTION(_VALUE)` 宏（或等价 helper）传播失败；不得手写分散的 `ToHandle(...)` 逻辑（除极少数特殊返回类型）。

## 2. Step 1：先在 runtime 层“全面铺开” MaybeHandle

### 2.1 现状确认与分类（只做一次）

对 `src/runtime/*.h` 内公开 API 进行分类：

* **确定为 Try/Lookup**：例如 `Runtime_FindPropertyInInstanceTypeMro/Runtime_FindPropertyInKlassMro`（现状返回 `Handle<PyObject>` 且 `null=未找到`）。

  * 动作：在对应头文件声明处补充明确注释：“返回 null 表示缺失，不会设置 pending exception”。

* **确定为 Fallible**：例如对象构造/执行/调用/反射（`Runtime_NewType/Runtime_NewObject/Runtime_NewDict/Runtime_NewSmi` 等，按实现行为判定）。

  * 动作：签名改为 `MaybeHandle<PyObject>`（或更具体类型），并用异常传播宏改造所有调用点。

### 2.2 改造原则（runtime）

* runtime 对外 API 只保留两种返回语义：

  * `MaybeHandle`：失败（一定是异常）

  * `Handle` + 注释：缺失（绝非异常）

* runtime 内部调用 `Execution::Call(...)`、`Runtime_NewStr(...)`、`Runtime_Execute*`、`Runtime_InvokeMagicOperationMethod` 等 Fallible API，一律用 `ASSIGN_RETURN_ON_EXCEPTION*` 宏传播。

### 2.3 交付与验收（runtime）

* runtime 头文件中所有 `Handle<PyObject>` 返回的 API，都有明确注释是否允许 `null=缺失`，并保证不抛错。

* runtime 中所有“可能抛错”的公开 API 改为 `MaybeHandle`。

* 编译 + ut 全量回归通过。

## 3. Step 2：在 builtins 层铺开 MaybeHandle（把 Python-facing API 类型化）

> 这一阶段的关键是：builtins 作为 Python 用户可调用的 native functions，目前大概率是 `Handle<PyObject>(*)(...)` 这种函数指针签名。要让 builtins 层全面使用 MaybeHandle，需要把 “native func ABI” 升级为 `MaybeHandle<PyObject>(*)(...)`，并在解释器调用 native builtin 时使用传播宏。

### 3.1 调整 builtins 的 native function ABI

* 修改 `PyFunction`（以及 native\_func 指针类型）：

  * 从 `Handle<PyObject>(*)(...)` 升级为 `MaybeHandle<PyObject>(*)(...)`

  * 这会波及 builtins 安装与解释器对 native 函数的调用路径

* 修改 builtins 安装点（`BuiltinBootstrapper::InstallBuiltinFunctions`）：

  * 使其注册的 native builtin 函数实现改为返回 `MaybeHandle<PyObject>`

### 3.2 builtins 函数实现改造（广度但可机械化）

* builtins 内部调用 runtime/Execution 的 Fallible API 时，用 `ASSIGN_RETURN_ON_EXCEPTION*` 宏简化与统一。

* builtins 的“正常返回 None”这类路径，返回 `handle(isolate->py_none_object())`。注意，Handle会隐式转换成MaybeHandle，因此为了代码简洁，请勿再显式调用MaybeHandle构造函数。

### 3.3 解释器对 native builtin 的调用路径（尽量小改）

* 在 `Interpreter::CallPythonImpl` 的 native function fast path：

  * 从直接接收 `Handle<PyObject>` 改为接收 `MaybeHandle<PyObject>` 并 `ToHandle(&result)`

  * 失败时直接返回 `kNullMaybe`（并依赖 pending exception 已设置）

* 调用点使用 `ASSIGN_RETURN_ON_EXCEPTION*` 宏，避免重复样板代码。

### 3.4 交付与验收（builtins）

* builtins 层 native functions 全部返回 `MaybeHandle<PyObject>`。

* builtins 层不再用 `Handle::null()` 表达“执行失败”。

* ut 全量回归通过，并补一个最小单测：builtin 抛错路径必须以 `MaybeHandle::IsEmpty()` 传播，且 pending exception 已设置。

## 4. Step 3（终极 Boss）：objects/vtable slot + PyObject API 全面 MaybeHandle 化

这是最大改动面，但放在最后做是正确的，因为此时 runtime/builtins 已经类型化，objects 的改动更容易“向上推”而不引入新的二义性。

### 4.1 vtable slot 迁移策略

* 将 `Klass::Virtual_Default_*` 中会触发执行/可能抛异常的 slot 返回值升级为 `MaybeHandle<PyObject>`（或 `MaybeHandle<PyString>` / `MaybeHandle<PyBoolean>`）。

* 对返回 `bool` 的“\*Bool”系列接口，避免“false 既可能是结果也可能是异常”的二义性：

  * 先造出 V8-LIKE 风格的 `Maybe<T>` 轮子

  * 将接口返回值修改为 `Maybe<T>`

### 4.2 PyObject 对外 API（py-object.h）迁移

* 将 `PyObject::Add/Sub/Mul/.../Len/GetAttr/SetAttr/Subscr/Iter/Next/Call/...` 等 **可能抛异常** 的 API 返回类型改为 `MaybeHandle<...>`。

* 将明确不会抛异常/仅查询的 API 保持为 `Handle/Tagged` 并写清注释。

### 4.3 调用点改造策略（广度最大，必须工具化）

* interpreter/runtime/builtins 内所有依赖 `PyObject::*` 的调用点，统一改为：

  * `ASSIGN_RETURN_ON_EXCEPTION`（函数本身返回 MaybeHandle 时）或 `ASSIGN_RETURN_ON_EXCEPTION_VALUE`

  * 或在解释器 handler 内 `if (maybe.IsEmpty()) goto pending_exception_unwind;`

* 只允许 Try/Lookup API 继续使用 `Handle::null()`，且调用点不得再用 pending exception 来区分语义。

### 4.4 交付与验收（objects）

* `py-object.h` 中所有“可能失败”的 API 均为 `MaybeHandle/Maybe`。

* objects 层不再通过 `Handle::null()` 表示执行失败。

* ut 全量回归通过，并补充最小单测覆盖：

  * PyObject 的算术/比较在异常时正确 unwind（不继续执行）

  * Try/Lookup API 的 null 返回不携带 pending exception

## 5. 分阶段执行顺序（每一步都可独立回归）

1. Step 1：runtime 全面铺开（含头文件注释与调用点宏化）→ 编译 + ut
2. Step 2：builtins native ABI 升级 + builtins 端到端 MaybeHandle → 编译 + ut
3. Step 3：objects/vtable/PyObject API 全面 MaybeHandle 化 → 编译 + ut

## 6. 关键风险与控制

* **签名扩散风险（Step 3）**：最大。控制：先完成 Step 1/2，把上层都变成 MaybeHandle 思维，再做 objects 扩散。

* **误把 Try/Lookup 变成 Fallible**：会导致调用点负担上升。控制：在头文件注释与 debug 断言中强约束“Try/Lookup 不得设置 pending exception”。

* **宏返回值不统一**：

  * 一般情况下，若调用方的返回值类型也是 `MaybeHandle<T>`，直接使用宏 `ASSIGN_RETURN_ON_EXCEPTION` 即可。它会返回 `kNullMaybe` 常量，而该常量可进一步隐式转换为`MaybeHandle<T>::null()` 。

  * 如调用方的返回值有其他特殊需求，使用宏 `ASSIGN_RETURN_ON_EXCEPTION_VALUE`，以便手工指定具体返回值。
