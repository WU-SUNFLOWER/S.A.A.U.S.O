# Step 2 完善计划：对齐 CPython 报错文案 + 引入 MaybeHandle 的工程化用法

## 背景与问题陈述

你指出的两个问题都成立：

1. 当前 C++ 抛错 API 已能“抛出并传播”，但缺少 CPython 风格的错误文案（message），导致用户在捕获/未捕获时都难以理解错误原因。
2. 仓库已有 `MaybeHandle<T>`，但当前新增的抛错链路主要采用“返回 null + pending exception”协议，未利用 `MaybeHandle` 的显式性优势。

本计划的目标是：在不引入 C++ 语言级异常的前提下，让错误体验尽可能对齐 CPython 3.12，同时把 `MaybeHandle` 纳入“VM 内部可持续迁移”的工程规范。

## 目标（本轮要交付）

* 让 `TypeError/RuntimeError/...` 这类错误在 Python 层具备可读 message（对齐 `str(e)` 行为），并在“未捕获异常”场景打印 `TypeError: ...` 的风格输出。

* 升级抛错 API：支持 `ThrowTypeError(message)`、`ThrowTypeErrorf(fmt, ...)` 等便捷接口，避免业务代码手写字符串拼接。

* 给出并落地 `MaybeHandle` 的使用场景：优先用于 **内部 helper**（builtins/runtime/interpreter 内部流程拆分函数），提升“调用方必须检查失败”的可读性与可维护性。

* 选取 3\~5 个高频错误点位（以 `exec/__build_class__/isinstance/runtime-exec` 为主）补齐“对齐 CPython 文案”的覆盖，并补充单测。

## 非目标（本轮不做）

* 不追求 CPython 完整 traceback/exception chaining（`__cause__`/`__context__`）一次对齐。

* 不把全仓库所有 `fprintf+exit` 一次性替换完（继续按“用户可触发路径优先”分批迁移）。

* 不引入新的泛型 `Maybe<T>`（目前仅使用已有 `MaybeHandle<T>`）。

## 设计方案

### A. 异常对象的“message”语义对齐

#### A1. 约定：异常对象持有 message（不包含前缀）

* 约定异常实例的 message 为 **不带异常类型前缀** 的文本：

  * `TypeError: keywords must be strings` 的 message 为 `keywords must be strings`

  * `TypeError: exec() got multiple values for argument 'globals'` 的 message 为 `exec() got multiple values for argument 'globals'`

* 这样可对齐 CPython：

  * `str(e)` 返回 message

  * 未捕获打印时由宿主统一拼 `TypeError: `  前缀

#### A2. 存储位置与 key 常量化

* 推荐将 message 存在异常实例的 `__dict__`（即 `PyObject::GetProperties(exc)`）里，key 统一为 `"message"`。

* 为避免频繁分配 key 字符串，计划把 `"message"` 加入 `StringTable`（类似 `__name__/__str__`），提供 `ST(message)`。

#### A3. BaseException 的 `__str__` / `__repr__`（最小语义闭环）

当前异常类型是用 `Runtime_CreatePythonClass` 动态建出来的，默认没有 `__str__`，这会导致 `print(e)`/`str(e)` 走不到 message。

计划在 `BuiltinBootstrapper::InstallBuiltinBasicExceptionTypes()` 创建 `BaseException` 后，在其 class dict 中注入：

* `BaseException.__str__(self)`：

  * 若 `self.__dict__['message']` 存在且为 `str`，返回它；

  * 否则返回空字符串或类型名（按 CPython 语义优先空字符串）。

* `BaseException.__repr__(self)`（MVP）：

  * 返回 `"<TypeName: message>"` 或 `"<TypeName>"`，用于调试；不追求完全对齐 CPython 的 `TypeError('msg')` repr。

实现形式：使用 native function（`NativeFuncPointer`）创建 `PyFunction`，直接塞进 BaseException 的 dict。

### B. 抛错 API：支持 message 与格式化

在 `Interpreter` 上补齐以下接口（不改变“pending exception 为 error indicator”的核心协议）：

* `Throw(Handle<PyObject> exception)`：保持现状。

* `ThrowNewException(type_name, message_or_null)`：保持现状但约定 message 语义为“不带前缀”。

* 新增便捷接口（核心提升点）：

  * `ThrowTypeError(Handle<PyString> message)` / `ThrowTypeError(std::string_view message)`

  * `ThrowRuntimeError(...)` 同理

  * `ThrowTypeErrorf(const char* fmt, ...)`：用 `vsnprintf` 组装 message，减少业务代码 string 拼接。

并建立约定：**新的错误路径不得只 ThrowTypeError() 空 message**，除非 CPython 原本也为空（极少）。

### C. 未捕获异常的“用户可见输出”

为了让用户看到 `TypeError: ...` 形式的报错，需要明确“未捕获异常”的归宿：

* 单测框架：`BasicInterpreterTest::RunScript` 在发现 pending exception 时，失败信息应包含：

  * 异常类型名（Klass name）

  * message（若存在）

  * 格式：`TypeError: keywords must be strings`

* CLI 入口（`src/main.cc`）：执行结束若有 pending exception，应打印同样格式并以非 0 退出码返回。

为避免重复拼接逻辑，计划新增一个统一 helper：

* `Interpreter::FormatPendingExceptionForStderr()` → `Handle<PyString>`

  * 内部读取异常类型名 + message，统一格式化。

### D. `MaybeHandle<T>` 的用武之地：为什么之前没用，以及如何用

#### D1. 为什么你目前看到“没用上”

* builtins 的 C++ ABI（`NativeFuncPointer`）固定返回 `Handle<PyObject>`，无法直接把 builtins 的对外签名改为 `MaybeHandle<PyObject>`（会牵涉大面积改动）。

* 当前链路选择了 CPython/C-API 风格的 MVP 协议：**失败返回 null + error indicator（pending exception）**，这在解释器内部已经足够表达“成功/失败”。

#### D2. 计划中的引入方式（推荐落点：内部 helper）

`MaybeHandle` 最适合用于两类场景：

1. **内部 helper 的阶段性拆分**：例如 `NormalizeExecArgs`、`ParseKwargs`、`GetDictOrTypeError` 这类“可能失败且需要调用方显式检查”的函数。

* 具体做法：

  * 把 helper 返回值从 `Handle<T>` / `bool` 逐步替换为 `MaybeHandle<T>`

  * 失败时：返回 `MaybeHandle<T>::Null()`，并通过 `Interpreter::ThrowTypeError(message)` 设置 pending exception

  * 调用方：`auto x = Helper(...); if (x.is_null()) return Handle<PyObject>::null();`

1. **跨模块 API 的可读性增强**：runtime 内部一些“会失败”的 API 可逐步增加 `MaybeHandle` 版本（保留旧版兼容），用于新代码路径优先采用。

本轮计划至少会在 `exec` 的若干解析 helper 中引入 1\~2 个 `MaybeHandle` 版本，形成“可复制模板”。

## 工程实施步骤（按最短闭环）

1. 增强异常 message 语义

   * 给 StringTable 增加 `message` 常量（`ST(message)`）。

   * 调整 `Interpreter::NewExceptionInstance`：写入 message（不带前缀）。

   * 在 builtin bootstrap 阶段给 `BaseException` 注入 `__str__/__repr__`。

2. 扩展抛错 API

   * 在 `Interpreter` 增加 `ThrowTypeError(message)`、`ThrowRuntimeError(message)`、`ThrowTypeErrorf`。

   * 增加 `FormatPendingExceptionForStderr()`（供单测与 main 复用）。

3. 把已迁移点位补齐“对齐 CPython 文案”

   * `builtins-exec.cc`：

     * `keywords must be strings`

     * `exec() got multiple values for argument 'globals'`

     * `exec() got an unexpected keyword argument 'xxx'`

     * `exec() globals must be a dict, not <type>`

   * `builtins-reflection.cc`（isinstance/build\_class）：

     * 对齐 `isinstance expected 2 arguments`、`isinstance() arg 2 must be a type, a tuple of types, or a union`（MVP 可先用较短版本，但需可读）

     * `__build_class__` 的错误也补齐 message（避免只抛空 TypeError）。

   * `runtime-exec.cc`：locals/globals/code 为 null 的错误补齐 message（并保持可捕获）。

4. 引入 MaybeHandle 试点

   * 在 `builtins-exec.cc` 将 1\~2 个 parsing helper 改为返回 `MaybeHandle<PyDict>`/`MaybeHandle<PyObject>`，并在失败时 Throw + 返回 Null。

   * 在代码中形成统一模板，后续迁移更多 helper。

5. 单测与回归

   * 新增/调整解释器端到端单测：

     * 捕获 TypeError 后 `print(str(e))` 与 message 断言；

     * 未捕获异常在测试框架中失败信息包含 `TypeError: ...`（可用 `ASSERT_DEATH` 不再适合这一类路径）。

   * 运行 `ut` 全量回归，确保无行为回退。

## 交付验收标准

* 典型错误场景用户可看到接近 CPython 的报错信息（至少包含异常类型名 + message）。

* `str(e)` 在 BaseException 子类上可工作，能拿到 message（MVP）。

* 已迁移的 `exec/__build_class__/isinstance/runtime-exec` 错误路径不再 `exit(1)`，且 message 与单测对齐。

* 新增 1\~2 个 `MaybeHandle` 试点 helper，形成后续可扩展的迁移模板。

