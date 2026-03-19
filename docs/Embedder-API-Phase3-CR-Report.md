# S.A.A.U.S.O Embedder API Phase 3 架构与代码审查报告 (CR)

## A. 审查总评

**总体等级：A-**
**结论：我同意进入下一阶段（Phase 4 或 Phase 3.1 重构），但需明确后续的架构还债计划。**

Phase 3 的交付质量很高。你利用了受限的 VM 内部机制（Trampoline 槽位）巧妙地完成了 C++ 与 Python 回调的互通，且 `FunctionCallbackInfo` 的栈上分配设计既优雅又安全。异常穿透链路（从 C++ 抛错到 Python 虚拟机再到外部 TryCatch）完整闭环，证明了前两个 Phase 的基础设施非常扎实。
当前代码不存在阻碍流程的 P0 致命缺陷，但因受限于早期 VM 的底层接口设计，留下了多线程竞争与内存膨胀的 P1 隐患，需要在下一阶段尽早根除。

---

## B. 架构亮点 (做得好的地方)

1. **栈上分配 Info 对象**：`FunctionCallbackInfo` 及其 `Impl` 直接在 `InvokeEmbedderCallbackAtSlot` 的 C++ 栈上分配，生命周期严格绑定回调执行期，无任何内存泄漏风险。
2. **异常穿透完美闭环**：`ThrowRuntimeError` -> 写入 `ExceptionState` -> Trampoline 拦截返回 `kNullMaybeHandle` -> Python 虚拟机 Unwind -> 外层 `TryCatch` 捕获，整个链路逻辑严密，无断层。
3. **空值安全处理**：`ToInternalObject` 中优雅地将空的 `Local<Value>`（如未设置返回值时）转换为 Python 的 `None`，完全符合 Embedder API 的直觉。
4. **测试分层严谨**：不仅测试了 Happy Path，还对参数校验失败、主动抛错等分支进行了覆盖，前后端测试分离验证，非常稳健。

---

## C. 问题与风险清单

### P1 风险（不阻碍当前流程，但下一阶段必须优先偿还的技术债）

#### 1. VM NativeFuncPointer 签名缺陷导致的回调槽位瓶颈与线程安全隐患
* **问题描述**：当前 `g_embedder_callback_slots` 是全局静态数组（大小16），且未加锁。这是因为 VM 的 `NativeFuncPointer` 签名未传入 `PyFunction` 本身或闭包数据（data），导致 C++ 侧只能通过静态函数的硬编码 index 来路由。
* **影响**：多 Isolate 并发时存在严重的线程竞争（Data Race）；16 个槽位的硬限制无法满足实际业务（如几百个 API 的导出）。
* **证据**：`api.cc` 中的 `g_embedder_callback_slots` 全局变量与宏展开的 Trampoline。
* **建议修复（Phase 3.1）**：必须修改 `src/objects/native-function-helpers.h` 中的 `NativeFuncPointer` 签名，增加 `Handle<PyObject> closure_data` 或将 `PyFunction` 自身传下，从而彻底废弃 16 个全局槽位的 Hack 实现。

#### 2. 回调高频执行导致外层 HandleScope 内存膨胀
* **问题描述**：在 Callback 内部调用 `info.GetIntegerArg()` 时，内部会调用 `operator[]`，进而触发 `WrapRuntimeResult`，在堆上分配新的 `Value` 包装对象并挂载到 Isolate 注册表。
* **影响**：由于 Callback 内部没有独立的 `HandleScope`，如果 Python 脚本在一个 `for` 循环中调用 C++ 回调 10 万次，将会导致 10 万个 `Value` 包装对象在外层 `HandleScope` 中堆积，引发脚本执行期的内存暴涨（OOM）。
* **证据**：`api.cc` 中 `FunctionCallbackInfo::operator[]` 返回了新创建的 `Local<Value>`。
* **建议修复（Phase 4）**：引入 `saauso::EscapableHandleScope`。在 `InvokeEmbedderCallbackAtSlot` 中包裹一个局部的 Scope，并在回调结束后，将 `return_value` “逃逸”回外层，释放回调期间产生的所有临时 `Value`。

---

## D. 测试补充建议

**已覆盖充分项**：`Context` 互调、回调注册、异常穿透（HostMock、CallbackThrow）均有独立的 Happy/Sad Path 断言。

**缺失项（建议作为技术债在后续补齐）**：
1. `Callback_Slot_Exhaustion_Test`：循环注册 17 个 Function，验证第 17 个是否正确触发 RuntimeError 异常，且 `TryCatch` 能够捕获。
2. `Context_Get_Miss_Test`：调用 `Context::Get` 查询不存在的 Key，断言返回空的 `MaybeLocal` 而不抛错。

---

## E. 下一阶段 Guardrail（必须遵守）

在进入 Phase 4（扩展 Object/List/Tuple 等复杂类型与能力完善）时，请严格遵守以下红线：
1. **彻底废弃 Trampoline Hack**：从 VM 底层重构 Native Function 签名，支持上下文数据（closure）透传，移除全局槽位。
2. **引入 EscapableHandleScope**：解决 Callback 内部及深层递归中的内存膨胀问题，完善 RAII 内存管理版图。
3. **保持 ABI 隔离**：在扩展 `Object` 等容器类型映射时，继续保持 `Value` 体系的纯净性，绝对禁止在 `saauso-embedder.h` 引入任何 `src/` 头文件。
