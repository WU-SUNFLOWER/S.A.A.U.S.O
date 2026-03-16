# 内建方法直接调用统一 receiver 适配层升级计划

## 1. Summary

目标：在不为每个 builtin 方法重复写 `if (self.is_null())` 的前提下，统一支持 CPython 风格的“通过类型直接调用方法”（如 `str.upper(s)`、`list.append(l, x)`），并消除当前崩溃风险。\
核心策略：在调用 pipeline 增加统一“receiver 归一化 + 描述符所有者校验”层，只对“内建方法描述符”生效，不影响全局 builtins 与模块函数。

***

## 2. Current State Analysis

### 2.1 当前调用链（已确认）

1. `LOAD_ATTR/CALL` 进入调用链：\
   `src/interpreter/interpreter-dispatcher.cc`
2. 调用前的 callable/receiver 归一化：\
   `Interpreter::NormalizeCallable` 位于 `src/interpreter/interpreter.cc`
3. Native 函数执行存在三处入口：

   * `Interpreter::CallPythonImpl`（fast path）

   * `Interpreter::InvokeCallable`（fast path）

   * `NativeFunctionKlass::Virtual_Call`（兜底虚调用）
4. 内建方法注册统一在：\
   `src/builtins/builtins-utils.cc::InstallBuiltinMethodImpl`

### 2.2 崩溃根因（已确认）

1. 大量内建方法体直接 `Handle<T>::cast(self)`，例如：\
   `src/builtins/builtins-py-list-methods.cc`、`src/builtins/builtins-py-string-methods.cc`
2. 对“类型对象属性访问 + 直接调用”场景，`self` 常为 `null`（或非实例对象），当前无统一补位与类型保护。
3. 结果是进入方法体后发生非法 cast，触发崩溃（而非抛出 `TypeError`）。

### 2.3 现有零散补丁现状

`__new__/__init__/部分 __repr__/__str__` 已在个别 builtin 文件内做了局部 `self` 兜底，但模式分散、重复、不可扩展。

***

## 3. Proposed Changes

### 3.1 统一方案总览

新增一层“Native 方法调用适配器”（建议命名：`NormalizeNativeMethodCall`），职责仅两件事：

1. **receiver 补位**：当调用目标是“内建方法描述符”且 `receiver == null` 时，从 `args[0]` 提取 receiver，并将位置参数左移（去掉首参）。
2. **receiver 校验**：根据描述符元数据检查 receiver 是否为 owner type（含子类）实例；失败时抛 `TypeError`，禁止进入具体 builtin 方法体。

该层执行后，具体 builtin 方法继续保持“假设 self 已合法”的实现风格，避免重复判空样板代码。

### 3.2 元数据建模（关键）

在 `PyFunction/FunctionTemplateInfo` 增加“原生函数调用语义元数据”，用于区分：

1. 普通 native 函数（如 `print/len/math.*`）
2. 内建方法描述符（如 `list.append/str.upper`）

建议元数据字段：

1. `native_call_kind`：`kPlainFunction | kMethodDescriptor`
2. `descriptor_owner_type`：指向 owner `PyTypeObject`（可空，仅 `kMethodDescriptor` 使用）

关联文件：

* `src/objects/templates.h`

* `src/objects/py-function.h`

* `src/heap/factory.h`

* `src/heap/factory.cc`

* `src/builtins/builtins-utils.h`

* `src/builtins/builtins-utils.cc`

### 3.3 方法安装链路改造

扩展 `InstallBuiltinMethodImpl`，在安装内建方法时写入上述元数据。\
为保证 owner type 可用，改造各 builtin-method `Install` 签名，显式传入 `owner_type`。

涉及：

1. 各 `builtins-py-*-methods.h/.cc` 的 `Install` 签名
2. 对应 klass 初始化调用点（`src/objects/py-*-klass.cc`）

注意：

* `BuiltinBootstrapper::InstallBuiltinFunctions` 安装的全局 builtins 仍走 `kPlainFunction`。

* 模块函数（math/time/random）保持 `kPlainFunction`。

### 3.4 调用 pipeline 注入点

将统一适配器接入以下所有 native 调用入口，保证路径全覆盖：

1. `Interpreter::CallPythonImpl` 的 native fast path
2. `Interpreter::InvokeCallable` 的 native fast path
3. `NativeFunctionKlass::Virtual_Call`

建议实现位置：

* 新增/扩展 `src/runtime/runtime-py-function.h/.cc`（放置公共适配逻辑）

* 由三个入口复用，避免逻辑分叉

### 3.5 错误语义对齐策略

当 `kMethodDescriptor` 调用不满足前置条件时，统一抛 `TypeError`，优先覆盖两类错误：

1. 缺失 receiver（`args` 为空）
2. receiver 类型不匹配（非 owner type 及其子类）

错误文案先遵循当前工程已用风格（descriptor needs an argument / requires '<type>' object），后续可再做 CPython 文案精修；先确保行为正确且不崩溃。

### 3.6 渐进收敛（可并入同一 PR 或后续 PR）

在统一适配层稳定后，可逐步删除 builtin 文件中重复的 `self.is_null()` 分支，仅保留具备独立语义的参数校验代码，降低冗余。

***

## 4. File-level Change Plan

### A. 元数据与对象模型

1. `src/objects/templates.h`

   * 扩展 `FunctionTemplateInfo`，携带 native call kind + owner type
2. `src/objects/py-function.h`

   * 为 native 函数对象增加对应元数据访问接口
3. `src/heap/factory.h`、`src/heap/factory.cc`

   * `NewPyFunctionWithTemplate` 写入元数据

### B. 内建方法安装

1. `src/builtins/builtins-utils.h`、`src/builtins/builtins-utils.cc`

   * `InstallBuiltinMethodImpl` 接受 owner type 并标记 `kMethodDescriptor`
2. `src/builtins/builtins-py-*-methods.h/.cc`

   * `Install` 透传 owner type
3. `src/objects/py-*-klass.cc`

   * 调用 `Install` 时传入本类型 `type_object()`

### C. 调用适配层

1. `src/runtime/runtime-py-function.h/.cc`

   * 新增统一 `NormalizeNativeMethodCall`（命名可调整）
2. `src/interpreter/interpreter.cc`

   * 两个 native fast path 调用该适配器
3. `src/objects/py-function-klass.cc`

   * `NativeFunctionKlass::Virtual_Call` 调用同一适配器

### D. 回归测试

1. `test/unittests/interpreter/interpreter-builtins-seq-unittest.cc`

   * 新增 `str.upper(s)`、`list.append(l, x)` 的“通过类型直接调用”成功路径

   * 新增错误路径：缺失 receiver、receiver 类型不匹配应抛 `TypeError`
2. 可补充脚本回归：`test/python312/call_method_of_builtin_type.py`

   * 增加直接类调用样例，确保不崩溃

***

## 5. Assumptions & Decisions

1. 本轮目标优先级：**先统一行为并消除崩溃**，再逐步精修错误文案。
2. 统一适配层只处理“内建方法描述符”；普通函数语义不变。
3. receiver 类型判断采用“owner type 或其子类”规则，以匹配 descriptor 常见语义。
4. 不引入新的 bytecode 语义，不改 `LOAD_ATTR/CALL` 协议，仅在 runtime call pipeline 收敛。

***

## 6. Verification

### 6.1 单元测试

1. 运行解释器相关单测（至少包含 `interpreter-builtins-seq-unittest`）。
2. 新增用例断言：

   * `str.upper("hello")` 与 `str.upper(s)`（成功）

   * `list.append(l, 233)`（成功）

   * `str.upper()`（TypeError）

   * `list.append(1, 2)`（TypeError）

### 6.2 回归验证

1. 复测现有 builtins 调用路径（实例方法、全局 builtins、模块 native 函数）无行为回退。
2. 校验无崩溃：直接类型调用 builtin 方法在错误输入下应稳定抛异常。

### 6.3 工程质量

1. 全量编译通过（Windows/Linux 现有告警策略下保持 clean）。
2. 不新增散落的 per-method `self.is_null()` 样板分支。

