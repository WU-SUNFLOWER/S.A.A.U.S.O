# 架构治理计划：将传播态异常状态下沉到 Isolate（ExceptionState），并将高层抛错 API 解耦到 runtime

## 目标形态（长期合理形态）

### 分层职责（对齐 V8-like）

* **execution / Isolate**

  * 持有 **ExceptionState**（error indicator）：`pending_exception` + `pending_exception_pc` + `pending_exception_origin_pc`。

  * 提供最薄能力：`Throw(exception)` / `HasPendingException()` / `Clear()` / 读取 pending 信息。

  * 为 runtime 提供必要的“通道”API（例如 `CallPython` 的转发），使 runtime 不需要 include interpreter。

* **interpreter**

  * 只“消费” ExceptionState：在执行循环中检查 pending，并做 zero-cost unwind（exception table 查表、切栈、跨帧传播）。

  * 继续持有 **caught exception state**（except-handler 期间的捕获态栈），因为这是字节码语义与 handler 进入/退出强耦合的执行细节。

* **runtime**

  * 负责 **高层异常 API**（构造异常对象、拼接 message、格式化输出）：`Runtime_ThrowTypeErrorf`、`Runtime_NewExceptionInstance`、`Runtime_FormatPendingExceptionForStderr` 等。

  * 仅通过 `Isolate::Current()->exception_state()` 设置 error indicator，不直接依赖 interpreter 的字段/实现。

* **builtins**

  * 继续遵守 `Handle<PyObject>` ABI（NativeFuncPointer），失败返回 `null` + error indicator 已设置。

  * 抛错统一走 runtime（避免 builtins 直接操作 ExceptionState 或 interpreter 细节）。

## 迁移范围（本轮要完成的代码改造）

### 1) 新增 ExceptionState（execution 层）

* 新增文件：`src/execution/exception-state.h`（必要时再加 `.cc`）。

* 内容：

  * 字段：`Tagged<PyObject> pending_exception_`、`int pending_exception_pc_`、`int pending_exception_origin_pc_`

  * 方法：

    * `bool HasPendingException() const`

    * `Tagged<PyObject> pending_exception_tagged() const` / `Handle<PyObject> pending_exception() const`

    * `void Throw(Tagged<PyObject> exc)`：只负责设置 pending（不构造对象），并重置 pc/origin 为 invalid

    * `void Clear()`

    * `int pending_exception_pc() const` / `void set_pending_exception_pc(int)`

    * `int pending_exception_origin_pc() const` / `void set_pending_exception_origin_pc(int)`

  * GC：提供 `Iterate(ObjectVisitor*)` 访问 `pending_exception_`。

### 2) Isolate 持有 ExceptionState，并暴露访问入口

* 修改 `src/execution/isolate.{h,cc}`：

  * 增加成员 `ExceptionState exception_state_`

  * 增加方法 `ExceptionState* exception_state()`

  * 在 `Isolate::Iterate`（或等价 GC root 扫描入口）里纳入 exception\_state 的 pointer。

  * 为降低 runtime 对 interpreter 的 include 需求，新增 “桥接”方法（只做转发，不放语义）：

    * `Handle<PyObject> CallPython(...)`（转发到 interpreter）

    * `Handle<PyDict> builtins()`（转发到 interpreter）

### 3) interpreter 仅消费 ExceptionState（移除 pending 字段）

* 修改 `src/interpreter/interpreter.{h,cc}`：

  * 删除或私有化 pending 相关字段（转移到 ExceptionState）。

  * 保留 `caught_exception_` 族字段与 `PUSH_EXC_INFO/POP_EXCEPT` 语义。

  * `HasPendingException()/pending_exception()/ClearPendingException()` 等对外接口改为委托给 `isolate_->exception_state()`（或直接移除、并逐点替换调用方）。

* 修改 `src/interpreter/interpreter-dispatcher.cc`：

  * `DISPATCH()` 的 pending 检查：改为查 `isolate_->exception_state()->HasPendingException()`

  * `pending_exception_unwind:` 中涉及 `pending_exception_pc_ / origin_pc_ / pending_exception_tagged()` 的逻辑改为读取/写入 ExceptionState。

  * 各 opcode handler 中设置 pending pc/origin 的点（如 `RaiseVarargs/CheckExcMatch` 等）改为写入 ExceptionState。

### 4) 高层抛错/格式化 API 上移到 runtime

* 新增文件：`src/runtime/runtime-exceptions.{h,cc}`

  * `Handle<PyObject> Runtime_NewExceptionInstance(Handle<PyString> type_name, Handle<PyString> message_or_null)`

  * `void Runtime_ThrowNewException(Handle<PyString> type_name, Handle<PyString> message_or_null)`

  * `void Runtime_ThrowTypeError(const char* message)` / `void Runtime_ThrowTypeErrorf(const char* fmt, ...)`

  * `Handle<PyString> Runtime_FormatPendingExceptionForStderr()`：读取 ExceptionState 的 pending，拼 `TypeName: message`

  * message 约定：**不包含** `TypeError: `  前缀

  * message 存储 key：继续使用 `ST(message)`

* 将目前 `Interpreter::{NewExceptionInstance, ThrowTypeErrorf, FormatPendingExceptionForStderr...}` 的实现迁移/改造到 runtime，interpreter 只保留最底层的“消费 pending”逻辑。

### 5) 调整调用方以解除 runtime/builtins 对 interpreter 的 include

* builtins：把 `ThrowTypeError/ThrowTypeErrorf/ThrowRuntimeError` 调用改为 `Runtime_ThrowTypeError*` / `Runtime_ThrowNewException`。

  * 重点修正：`src/builtins/builtins-exec.cc`、`src/builtins/builtins-reflection.cc`

* runtime：把 `Isolate::Current()->interpreter()->Throw...` 迁移为 `Runtime_Throw...`

* 对于 runtime 中需要调用 Python 的路径（例如 `runtime-py-string.cc` 里调用 `CallPython`），改为走 `Isolate::Current()->CallPython(...)` 的转发接口，从而移除对 `interpreter.h` 的 include。

### 6) 单测与 CLI 输出对齐

* `test/unittests/test-helpers.cc`：未捕获异常信息格式化改为 `Runtime_FormatPendingExceptionForStderr()`（不再通过 interpreter）。

* `src/main.cc`：同上，避免依赖 interpreter 的高层格式化 API。

* 确保 `except ... as e` 相关字节码仍可工作（目前已补 `DELETE_NAME`，需回归验证）。

## 风险点与治理策略

* **循环依赖风险**：runtime 不应 include interpreter；通过 `Isolate` 的桥接方法解决 “runtime 需要 CallPython/builtins” 的需求。

* **状态一致性风险**：pending pc/origin 的单位与写入时机必须保持现状（byte offset）；迁移后重点回归 `interpreter-exceptions-unittest`。

* **API 迁移成本**：先保持对外行为不变（失败返回 null + pending），只移动“状态归属”和“高层构造/文案”。

## 验证步骤（实施后必须完成）

* `gn gen out/ut` + `ninja -C out/ut ut` + 运行 `out/ut/ut.exe`

* 重点关注：

  * `interpreter-exceptions-unittest.cc`（异常展开、re-raise、finally）

  * `interpreter-exec-unittest.cc`（exec 报错 message）

  * `interpreter-custom-class-unittest.cc`（__build\_class__ 报错 message 与 `except ... as e`）

## 交付验收标准

* pending exception 状态不再存放在 Interpreter 内部，而是下沉到 Isolate 的 ExceptionState。

* runtime/builtins 不需要 include interpreter 即可抛错与格式化错误输出（通过 runtime + Isolate 桥接）。

* 全量 `ut` 通过，且现有已对齐的报错文案保持不回退。

