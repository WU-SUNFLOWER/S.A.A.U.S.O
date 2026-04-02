# Embedder API 基于 Maybe 的统一改造计划

## Summary

* 目标：基于已迁移到 public API 层的 `Maybe`（`include/saauso-maybe.h`），统一改造 Embedder API 返回值语义，减少 `bool + out-param`、`bool`、空 `Local` 混用带来的心智负担。

* 范围：覆盖 `include/saauso-*.h` 与 `src/api/*.cc` 中所有“相关可失败 API”签名与实现，并同步修改 embedder 单测。

* 约束：允许破坏性签名变更；`Get` 类接口继续保持 `MaybeLocal` 不区分 miss/error；`ToXxx/GetXxxArg` 改为 `Maybe<T>`；`Set/Push` 改为 `Maybe<void>`；“可失败且返回 Local”的接口统一改为 `MaybeLocal`。

## Current State Analysis

* 公共 `Maybe` 已存在于 [saauso-maybe.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-maybe.h)。

* 当前返回值语义存在混用：

  * `MaybeLocal`：如 `Script::Compile/Run`、`Function::Call`、`Object::Get`、`Context::Get`。

  * `bool + out-param`：`Value::ToString/ToInteger/ToFloat/ToBoolean`、`FunctionCallbackInfo::GetIntegerArg/GetStringArg`。

  * `bool`：`Context::Set`、`Object::Set`、`List::Push/Set`。

  * 可失败但返回 `Local`：`Function::New`、`Context::Global`、`Exception::TypeError/RuntimeError`、`TryCatch::Exception`。

* 调用方（尤其 `test/embedder/*.cc`）当前大量依赖 `if (!ok)` 和 `Local.IsEmpty()` 风格，需要同步迁移断言与解包逻辑。

## Proposed Changes

### 1) 公共头聚合与依赖关系

* 修改 [saauso.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso.h)

  * 新增 `#include "saauso-maybe.h"`，保证聚合头默认导出 Maybe 能力。

* 修改 [saauso-local-handle.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-local-handle.h)

  * 增补 `#include "saauso-maybe.h"`。

  * 新增 `template <typename T> using MaybeLocal = Maybe<Local<T>>;`，将 `MaybeLocal` 统一建立在公共 `Maybe` 基建上。

  * 删除当前自定义 `MaybeLocal` 类实现，避免双语义并存。

### 2) Embedder API 头文件签名统一（含使用说明注释）

* 修改 [saauso-value.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-value.h)

  * `bool ToString(std::string* out) const` -> `Maybe<std::string> ToString() const`

  * `bool ToInteger(int64_t* out) const` -> `Maybe<int64_t> ToInteger() const`

  * `bool ToFloat(double* out) const` -> `Maybe<double> ToFloat() const`

  * `bool ToBoolean(bool* out) const` -> `Maybe<bool> ToBoolean() const`

  * 为每个 API 增加“成功/失败（Nothing）语义”注释与典型调用说明。

* 修改 [saauso-function-callback.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-function-callback.h)

  * `bool GetIntegerArg(int index, int64_t* out) const` -> `Maybe<int64_t> GetIntegerArg(int index) const`

  * `bool GetStringArg(int index, std::string* out) const` -> `Maybe<std::string> GetStringArg(int index) const`

  * `Local<Value> operator[](int index) const` -> `MaybeLocal<Value> operator[](int index) const`

  * `Local<Value> Receiver() const` -> `MaybeLocal<Value> Receiver() const`

  * 增加参数越界/类型不匹配/空值时的返回约定注释。

* 修改 [saauso-context.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-context.h)

  * `static Local<Context> New(Isolate* isolate)` -> `static MaybeLocal<Context> New(Isolate* isolate)`

  * `bool Set(...)` -> `Maybe<void> Set(...)`

  * `Local<Object> Global()` -> `MaybeLocal<Object> Global()`

  * `Get` 保持 `MaybeLocal<Value>`（按已决策不区分 miss/error）。

  * 增加 `Enter/Exit/Set/Get/Global` 的使用说明注释。

* 修改 [saauso-object.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-object.h)

  * `static Local<Object> New(...)` -> `static MaybeLocal<Object> New(...)`

  * `bool Set(...)` -> `Maybe<void> Set(...)`

  * `Get/CallMethod` 保持 `MaybeLocal<Value>` 并补注释。

* 修改 [saauso-container.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-container.h)

  * `static Local<List> New(...)` -> `static MaybeLocal<List> New(...)`

  * `bool Push(...)` -> `Maybe<void> Push(...)`

  * `bool Set(...)` -> `Maybe<void> Set(...)`

  * `Tuple::Get/List::Get` 保持 `MaybeLocal<Value>`，补充 miss/error 统一为空的注释。

* 修改 [saauso-function.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-function.h)

  * `static Local<Function> New(...)` -> `static MaybeLocal<Function> New(...)`

  * `Call` 保持 `MaybeLocal<Value>`，补充参数与失败语义注释。

* 修改 [saauso-exception.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-exception.h)

  * `TypeError/RuntimeError`：`Local<Value>` -> `MaybeLocal<Value>`

  * `TryCatch::Exception`：`Local<Value>` -> `MaybeLocal<Value>`

  * 增加 `TryCatch` 使用流程说明注释。

* 修改 [saauso-script.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-script.h)

  * 保持签名不变，补充 `Compile/Run` 返回值与异常捕获配合注释。

### 3) API 实现同步改造

* 修改 [api-local-handle.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-local-handle.cc)

  * 适配 `MaybeLocal` 由 `Maybe<Local<T>>` 别名后的行为差异（若有）。

* 修改 [api-value.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-value.cc)

  * `ToXxx` 直接返回 `Maybe<T>`；成功返回 `Maybe<T>(value)`，失败返回 `kNullMaybe`。

* 修改 [api-function-callback.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-function-callback.cc)

  * `GetXxxArg`、`operator[]`、`Receiver` 改为 Maybe 风格链式返回与解包。

* 修改 [api-context.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-context.cc)

  * `New/Global` 改 `MaybeLocal`。

  * `Set` 改 `Maybe<void>`（成功 `JustVoid()`，失败 `kNullMaybe`）。

  * `Context::Scope` 内部适配 `MaybeLocal<Context>` 新调用方式。

* 修改 [api-objects.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc)

  * `Object::New` 改 `MaybeLocal<Object>`，`Set` 改 `Maybe<void>`。

* 修改 [api-container.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-container.cc)

  * `List::New` 改 `MaybeLocal<List>`，`Push/Set` 改 `Maybe<void>`。

* 修改 [api-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-function.cc)

  * `Function::New` 改 `MaybeLocal<Function>`。

* 修改 [api-exception.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-exception.cc)

  * `TypeError/RuntimeError` 与 `TryCatch::Exception` 改 `MaybeLocal<Value>`。

### 4) 调用方与单测迁移

* 修改 [embedder-script-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/embedder/embedder-script-unittest.cc)

* 修改 [embedder-interop-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/embedder/embedder-interop-unittest.cc)

  * 将 `bool + out-param` 校验迁移为 `Maybe<T>` 解包校验。

  * 将 `Local` 返回值判空迁移为 `MaybeLocal` 检查与 `ToLocal`。

  * 将 `Set/Push` 断言迁移为 `IsJust()/IsNothing()`。

## Assumptions & Decisions

* 决策：允许破坏性签名变更，不保留旧 API 兼容层。

* 决策：`Get` 类接口继续使用 `MaybeLocal`，不区分 miss 与异常失败。

* 决策：`ToXxx/GetXxxArg` 使用 `Maybe<T>`。

* 决策：`Set/Push` 使用 `Maybe<void>`。

* 决策：`saauso.h` 聚合导出 `saauso-maybe.h`。

* 决策：在所有受影响 `saauso-xxx.h` 中补齐面向嵌入方的 API 使用说明注释。

## Verification Steps

* 编译与测试：

  * 运行 `test/embedder:embedder_ut`，确保 Embedder API 签名迁移后全部通过。

  * 运行主回归 `test/unittests:ut`，确认无上层语义回归。

* 语义校验：

  * 覆盖 `Maybe<void>` 成功/失败路径（`Set/Push`）。

  * 覆盖 `Maybe<T>` 类型提取成功、类型不匹配、参数越界路径（`ToXxx/GetXxxArg`）。

  * 覆盖 `MaybeLocal` 空值路径与 `TryCatch` 协同行为（编译失败、运行异常、Get miss）。

* API 文档校验：

  * 检查所有修改过的 `include/saauso-*.h` 是否存在对应方法的注释，且与实现语义一致。

