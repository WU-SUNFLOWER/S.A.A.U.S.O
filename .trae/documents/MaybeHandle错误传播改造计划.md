# 架构/工程改造计划：以 MaybeHandle 强制表达“可能抛 Python 异常”的失败路径

## 0. 改造目标（你要解决的核心问题）

把“返回 `Handle<PyObject>::null()` 既可能表示缺失、也可能表示失败”的二义性拆开：

* **可失败（会抛 Python 异常）的 API**：必须返回 `MaybeHandle<T>`，调用方必须显式检查。

* **可缺失（try-get/lookup，不抛错）的 API**：继续返回 `Handle<T>`，`null` 仅表示缺失，并且必须保证不设置 pending exception。

并保持现有异常机制不变：失败时仍通过 `Isolate::exception_state()` 设置 pending exception（error indicator）。

## 1. 设计约定（必须写入规范并全仓库执行）

### 1.1 两类 API（强约束）

1. **Fallible API（可能抛异常）**

* 返回：`MaybeHandle<T>`

* 失败形态：返回 `MaybeHandle<T>::null()`，并且 **必须** 已设置 pending exception

* 调用方：必须检查 `IsEmpty()` 并向上返回 `MaybeHandle::null()` 或进入 unwind

1. **Try/Lookup API（不会抛异常）**

* 返回：`Handle<T>` 或 `Tagged<T>`

* `null` 语义：仅表示“没找到/没有结果”

* 强约束：此类 API **绝不能** 设置 pending exception

### 1.2 调用点模板（机械化改造，减少心智负担）

* 在新增文件 `src/execution/exception-utils.h` 中提供 V8-like 风格的 Macro Helper（头文件中需详细说明工具宏的用法，并给出例子）：

  ```C++
  #define ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, value)   \
    do {                                                                \
      if (!(call).ToHandle(&dst)) {                                     \
        assert((isolate)->->exception_state()->HasPendingException());  \
        return value;                                                   \
      }                                                                 \
    } while (false)

  #define ASSIGN_RETURN_ON_EXCEPTION(isolate, dst, call) \
    ASSIGN_RETURN_ON_EXCEPTION_VALUE(isolate, dst, call, kNullMaybe)

  #define RETURN_ON_EXCEPTION_VALUE(isolate, call, value)               \
    do {                                                                \
      if ((call).IsEmpty()) {                                           \
        assert((isolate)->->exception_state()->HasPendingException());  \
        return value;                                                   \
      }                                                                 \
    } while (false)

  #define RETURN_ON_EXCEPTION(isolate, call) \
    RETURN_ON_EXCEPTION_VALUE(isolate, call, kNullMaybe)
  ```

* Interpreter handler 内：`if (maybe.IsEmpty()) goto pending_exception_unwind;`

* 为 `MaybeHandle` 增加 `[[nodiscard]]`（若当前实现未标注），避免调用者忽略检查。

## 2. 迁移策略（分阶段，保证可回归）

> 目标是“先收敛入口、再扩散”，避免一次性改爆全仓库。

### Phase A：建立规则与最小基础设施（低风险）

1. 明确并更新规范文档：

* 在 `AGENTS.md` 增加/补充 **Error Conventions**：

  * Fallible API 必须返回 `MaybeHandle`

  * Try/Lookup API 返回 `Handle`，null=缺失且不抛错

  * “`MaybeHandle::IsEmpty()` 必须伴随 pending exception” 的强约束

1. 为 `MaybeHandle` 增强可用性：

* 若缺少：添加 `[[nodiscard]]`、`ASSIGN_RETURN_ON_EXCEPTION` 等小工具（不引入额外依赖）

验收：不改业务代码也能编译；新增工具在不使用时不影响行为。

### Phase B：收敛“执行/调用”入口（Execution/CallPython）

把调用执行器这条链路作为第一批试点，因为它是最通用的“可失败”入口：

* 修改 `Execution::Call(...)` 返回值：

  * 从 `Handle<PyObject>` 改为 `MaybeHandle<PyObject>`

* 解释器的调用入口（`Interpreter::CallPython` / `CallPythonImpl`）同步改为 `MaybeHandle`：

  * 解释器内部也改为 `MaybeHandle`，让 call 语义统一

验收：所有调用 `Execution::CallPython` 的点位全部完成机械替换与检查，不再把 `null` 当“普通返回值”。

### Phase C：迁移 runtime 层的“语义流程 API”

优先迁移最容易触发用户代码、最容易出错的 runtime helper：

* `Runtime_InvokeMagicOperationMethod`

* `Runtime_NewStr`

* `Runtime_Execute*`

* 以及 `klass-default-vtable.cc` 中默认 print/call/getattr 中会触发 CallPython 的路径

做法：凡是会触发执行/抛错的函数，签名改为 `MaybeHandle<PyObject>`；调用点用模板宏传播。

验收：runtime 层不再返回 “失败也返回 Handle::null()” 的混合语义。

### Phase D：迁移 builtins 层（参数解析 + 抛错）

* builtins 按“失败就返回 null()”的惯例改为 `MaybeHandle<PyObject>`：

  * **【正常情况】** 如果允许改 ABI：直接改为 `MaybeHandle`（需同步修改 builtin 注册表/调用约定）

  * **【极个别特殊情况】** 如果 builtin 的对外 ABI 必须是 `Handle<PyObject>`：在 builtin 末尾统一用 `if (pending) return null;` 的收敛点

本阶段优先改造已试点/高频路径：`exec/isinstance/__build_class__`。

### Phase E：扩大覆盖面 + 清理遗留

* 清理剩余“可失败但仍返回 Handle”的 API

* 把 try-get/lookup 家族统一命名（例如 `TryGetAttr`、`FindProperty...`），确保不会设置 pending

## 3. 代码改造清单（预计涉及文件范围）

### 基础设施

* `src/handles/maybe-handles.h`（增强 MaybeHandle：nodiscard）

* 新增：`src/runtime/exception-utils.h`（仅宏/inline helper，不引入新依赖）

* `AGENTS.md`（新增 Error Conventions 章节）

### 执行入口

* `src/execution/execution.{h,cc}`

* `src/interpreter/interpreter.{h,cc}`

* `src/interpreter/interpreter-dispatcher.cc`（在 handler 内按 MaybeHandle 传播）

### runtime/builtins

* `src/runtime/runtime-*.cc`（涉及 Call/str/magic/exec 的路径）

* `src/builtins/builtins-*.cc`（已触及异常路径的 builtins）

* `src/objects/klass-default-vtable.cc`

## 4. 风险与控制

* **签名扩散风险**：MaybeHandle 会扩大改动面。
  - 控制方式：Phase B/C 先集中在“执行入口/语义流程”这类关键链路，避免一次性全改。

* **语义回退风险**：如果出现 `MaybeHandle::null()` 但没设置 pending exception，会导致上层误判。
  - 控制方式：在 debug 下对 `ASSIGN_RETURN_ON_EXCEPTION_VALUE`和`RETURN_ON_EXCEPTION_VALUE`工具宏返回点加断言（例如 `ASSERT(pending)`）。

* **Try API 污染风险**：try-get/lookup 必须保证不抛错；需要逐步清理并写测试覆盖典型路径。

## 5. 验证策略（每个阶段都可运行）

* 编译：`gn gen out/ut` + `ninja -C out/ut ut`

* 运行：`out/ut/ut.exe`

* 增量新增单测（优先）：

  * “失败路径必须返回 MaybeHandle::Null() 且 pending 已设置”

  * “Try/Lookup 返回 null 时 pending 必须未设置”

  * 覆盖 `Execution::CallPython` 失败传播与 interpreter unwind 行为

## 6. 交付验收标准

* 仓库内对外约定明确：Fallible API 一律 `MaybeHandle`，Try/Lookup API 一律 `Handle` 且不抛错。

* 核心执行链路（Execution → interpreter → runtime/builtins）不再依赖 `Handle::null()` 的二义性。

* 全量 `ut` 通过，并新增至少一组针对“Null + pending”与“Try-null”区分的回归测试。

