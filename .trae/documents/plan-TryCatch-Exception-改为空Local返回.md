# Summary

* 目标：将 `TryCatch::Exception()` 从 `MaybeLocal<Value>` 调整为 `Local<Value>`，在“未捕获异常”时返回空 `Local<Value>`，在“已捕获异常”时返回有效 `Local<Value>`。

* 范围：仅收口 `TryCatch::Exception()` 这一处 Embedder API 的声明、实现、调用点与相关文档/测试，不顺带扩展到其他 `Local` / `MaybeLocal` 统一化工作。

* 成功标准：公开头、实现、示例、单测与文档语义一致；现有依赖 `Exception().ToLocal(...)` 的调用点全部迁移；相关 Embedder 测试通过。

# Current State Analysis

* `TryCatch::Exception()` 当前声明位于 `include/saauso-exception.h`，返回 `MaybeLocal<Value>`，注释为“未捕获时返回 Nothing”。对应实现位于 `src/api/api-exception.cc`，本质是读取 `impl_->exception_`，为空时返回空 `MaybeLocal<Value>`，否则返回 `api::Utils::ToLocal<Value>(exception)`。

* `Local<T>` 本身允许为空，并通过 `IsEmpty()` 判空；`MaybeLocal<T>` 额外提供 `ToLocal(...)` / `ToLocalChecked()` 解包语义。`TryCatch::Exception()` 当前并不执行可能失败的计算，而只是读取已缓存的异常对象，因此语义上更接近“可空 `Local` 访问器”。

* 现有调用点已确认分布在：

  * `samples/catch-python-exception.cc`

  * `test/embedder/embedder-interop-callback-unittest.cc`

  * `test/embedder/embedder-interop-value-unittest.cc`

  * `test/embedder/embedder-script-unittest.cc`

* 其中需要重点改写的是 `test/embedder/embedder-interop-value-unittest.cc` 里对 `Exception().ToLocal(&exception)` 的用法，改为直接接收 `Local<Value>` 并判空。

* 文档 `docs/Saauso-Embedder-API-User-Guide.md` 当前强调“可失败 API 返回空 `MaybeLocal<T>`”，但并未专门说明 `TryCatch::Exception()` 必须返回 `MaybeLocal<Value>`；因此只需补充/微调该 API 的使用语义，避免与新的签名形成心智冲突。

# Proposed Changes

## 1. 更新公开接口声明

* 文件：`include/saauso-exception.h`

* 改动：

  * 将 `TryCatch::Exception() const` 的返回类型从 `MaybeLocal<Value>` 改为 `Local<Value>`。

  * 将注释从“未捕获时返回 Nothing”改为“未捕获时返回空 Local<Value>”。

* 原因：

  * 该接口是读状态访问器，不是执行型 fallible API。

  * 与 `Local<T>` 的天然空值语义更匹配，调用端更直接。

## 2. 更新实现

* 文件：`src/api/api-exception.cc`

* 改动：

  * 将函数签名同步改为 `Local<Value> TryCatch::Exception() const`。

  * 保持现有逻辑不变：内部异常句柄为空时返回默认构造的空 `Local<Value>`，否则返回 `api::Utils::ToLocal<Value>(exception)`。

* 原因：

  * 行为不变，只收口返回类型与调用体验。

## 3. 迁移调用点

* 文件：`samples/catch-python-exception.cc`

  * 若只是“触发读取”而不使用结果，则改为显式接收返回值或直接删除无意义裸调用，避免保留旧心智模型。

* 文件：`test/embedder/embedder-interop-callback-unittest.cc`

  * 保留 `IsEmpty()` 断言即可，签名变更不影响测试意图。

* 文件：`test/embedder/embedder-interop-value-unittest.cc`

  * 将 `ASSERT_TRUE(try_catch.Exception().ToLocal(&exception));` 改为：

    * `Local<Value> exception = try_catch.Exception();`

    * `ASSERT_FALSE(exception.IsEmpty());`

  * 保持后续消息断言不变。

* 文件：`test/embedder/embedder-script-unittest.cc`

  * 仅保留 `IsEmpty()` 语义断言，无需结构性变更。

* 原因：

  * 让所有示例/测试体现新接口的直接使用方式。

## 4. 同步文档

* 文件：`docs/Saauso-Embedder-API-User-Guide.md`

* 改动：

  * 在 `TryCatch` / 错误处理相关段落补一句：`TryCatch::Exception()` 返回 `Local<Value>`；未捕获时为空句柄。

  * 不扩大到整体 `Local` / `MaybeLocal` 规则重写，只做与本次接口变更直接相关的同步。

* 原因：

  * 保持用户指南与头文件签名一致，避免嵌入方继续按 `ToLocal(...)` 理解该 API。

# Assumptions & Decisions

* 决策：本次仅调整 `TryCatch::Exception()`，不顺带修改 `Context::New`、`Function::New` 等其他返回空 `Local` / `MaybeLocal` 边界不完全一致的 API。

* 决策：不改变异常捕获链路、`HasCaught()` / `Reset()` 语义、`Global<PyObject>` 保活方式与任何 VM 内部异常传播逻辑。

* 假设：现有构建系统与测试目标不需要为该改动新增 BUILD 依赖或新测试文件，只需修改现有测试断言。

* 决策：优先保持局部最小改动，避免把此次 Embedder API 精校扩展成大范围风格治理。

# Verification Steps

* 静态检查：

  * 全仓搜索 `try_catch.Exception()` / `.Exception()`，确认无残留 `ToLocal(...)` 旧写法依赖该返回值。

  * 读取公开头与文档，确认注释/签名/示例一致。

* 构建与测试：

  * 重新构建包含 Embedder API 与测试的目标。

  * 运行至少以下相关测试：

    * `test/embedder/embedder-interop-value-unittest.cc`

    * `test/embedder/embedder-interop-callback-unittest.cc`

    * `test/embedder/embedder-script-unittest.cc`

  * 若仓库已有合并目标（如 `embedder_ut`），优先运行聚合测试目标。

* 验收标准：

  * 编译通过，无返回类型不匹配错误。

  * 相关测试通过。

  * `TryCatch::Exception()` 在代码、文档与测试中的使用方式统一为“直接返回可空 `Local<Value>`”。

