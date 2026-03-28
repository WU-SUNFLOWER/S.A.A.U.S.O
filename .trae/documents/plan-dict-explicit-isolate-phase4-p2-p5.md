# 【去Isolate::Current()化】【第四期】PyDict P2\~P5 推进计划

## Summary

* 目标：在已完成 `PyDict` 原语显式 `Isolate*` 化与 `PyDict::New(...)` 收敛的前提下，继续完成 P2\~P5：打通直接调用链、补齐 dict views / iterators、收口剩余接口风格、并完成定向验证。

* 范围：仅聚焦 `PyDict` 容器原语及其直接消费方，不把任务扩展为 `HandleScope`、全局 `IsXxx` checker 或 `Isolate::SetUp/TearDown` 的全局治理。

* 原则：

  * 不引入双轨过渡 API。

  * 已持有 `isolate` 的调用点直接透传。

  * 保持 runtime-py-dict.h 对 fallible API 的异常传播约束不变。

## Current State Analysis

### 已完成的基础状态

* `PyDict` 原语已显式 `Isolate*` 化：

  * `ItemAtIndex(int64_t, Isolate*)`

  * `ContainsKey(..., Isolate*)`

  * `Get / GetTagged(..., Isolate*)`

  * `Remove(..., Isolate*)`

  * `Put(..., Isolate*)`

  * `GetKeyTuple(..., Isolate*)`

  * `ExpandImpl(..., Isolate*)`

* 内部 helper 已同步显式化：

  * `FindSlot(..., Isolate*)`

  * `RehashInto(..., Isolate*)`

* `PyDict::New(Isolate*, int64_t)` 已存在：

  * [py-dict.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.h#L16-L19)

  * [py-dict.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.cc#L110-L113)

### 当前尚未闭合的直接调用链

* runtime 层仍大量按旧签名调用 `PyDict` 原语：

  * [runtime-py-dict.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-dict.cc#L24-L253)

  * [runtime-intrinsics.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-intrinsics.cc#L33-L134)

  * [runtime-reflection.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-reflection.cc)

  * [runtime-exec.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exec.cc)

  * [runtime-exceptions.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc)

* objects / klass / views 仍存在旧签名调用：

  * [py-dict-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-klass.cc#L82-L220)

  * [py-dict-views-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views-klass.cc#L36-L543)

  * [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)

  * [py-base-exception-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-base-exception-klass.cc)

  * [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)

* interpreter / modules / builtins / api 仍未完成透传：

  * [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc#L358-L504)

  * [frame-object-builder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object-builder.cc#L236-L345)

  * [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc#L63-L75)

  * [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)

  * [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)

  * [module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)

  * [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)

  * [builtins-py-dict-methods.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-py-dict-methods.cc)

  * [builtins-exec.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-exec.cc)

  * [builtins-io.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-io.cc)

  * [api-objects.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc#L55-L79)

  * [api-container.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-container.cc)

  * [api-context.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-context.cc)

### 需要特别注意的结构性问题

* `py-dict-views-klass.cc` 中 `NextFromIterator` 目前不接收 `isolate`，而 item iterator 的 getter 需要调用 `dict->ItemAtIndex(index, isolate)`；这意味着 P3 不能只改 lambda，必须先改 helper 形状：

  * [py-dict-views-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views-klass.cc#L36-L56)

  * [py-dict-views-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views-klass.cc#L487-L499)

* `ModuleUtils::IsPackageModule/GetPackagePathList` 当前返回裸 `bool`，内部直接调用 dict fallible API；P2 可以先透传 isolate，但不在本轮把它们升级成 `Maybe<bool>`，否则范围会扩成模块工具错误语义治理：

  * [module-utils.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.cc#L37-L69)

## Proposed Changes

### P2：打通 PyDict 直接调用链

#### 1) runtime 层统一透传 isolate

* 修改文件：

  * `src/runtime/runtime-py-dict.cc`

  * `src/runtime/runtime-intrinsics.cc`

  * `src/runtime/runtime-reflection.cc`

  * `src/runtime/runtime-exec.cc`

  * `src/runtime/runtime-exceptions.cc`

* 变更内容：

  * 所有 `dict->Get / GetTagged / ContainsKey / Remove / ItemAtIndex`
    改为显式传入 `isolate`

  * 所有 `PyDict::Put / PyDict::GetKeyTuple / PyDict::New`
    改为显式传入 `isolate`

* 目的：

  * 让 runtime 这一层成为“高层语义入口 + 显式 isolate 透传”的稳定样板。

#### 2) objects / klass 层补齐直连调用

* 修改文件：

  * `src/objects/py-dict-klass.cc`

  * `src/objects/py-object-klass.cc`

  * `src/objects/py-base-exception-klass.cc`

  * `src/objects/py-type-object-klass.cc`

  * `src/objects/klass.cc`

* 变更内容：

  * 所有消费 `PyDict` fallible 原语的点位补传 `isolate`

  * 所有类字典初始化点位从 `PyDict::NewInstance()` 切换到 `PyDict::New(isolate)`

* 目的：

  * 保证对象系统主干中不再出现“已拿到 isolate 却继续走隐式 current”的调用模式。

#### 3) interpreter / builtins / modules / api 补齐透传

* 修改文件：

  * `src/interpreter/interpreter-dispatcher.cc`

  * `src/interpreter/frame-object-builder.cc`

  * `src/interpreter/builtin-bootstrapper.cc`

  * `src/interpreter/interpreter.cc`

  * `src/builtins/builtins-py-dict-methods.cc`

  * `src/builtins/builtins-exec.cc`

  * `src/builtins/builtins-io.cc`

  * `src/modules/module-loader.cc`

  * `src/modules/module-importer.cc`

  * `src/modules/module-name-resolver.cc`

  * `src/modules/module-manager.cc`

  * `src/modules/builtin-module-utils.cc`

  * `src/api/api-objects.cc`

  * `src/api/api-container.cc`

  * `src/api/api-context.cc`

* 变更内容：

  * 已持有 `isolate` 或 `internal_isolate` 的地方直接透传

  * `PyDict::NewInstance()` 全部切换为 `PyDict::New(isolate)`

  * `PyDict::Put/Get/GetTagged/Remove/ContainsKey` 全部改成显式 isolate

* 目的：

  * 完成 P2“调用链闭环”，把第四期主线真正从 `py-dict.cc` 扩展到其直接消费方。

### P3：补齐 dict views / iterators

* 修改文件：

  * `src/objects/py-dict-views-klass.cc`

* 变更内容：

  * 将 `NextFromIterator` 改为显式接收 `Isolate*`

  * `Virtual_Contains` 中 `dict->ContainsKey(...)`、`dict->Get(...)` 补传 `isolate`

  * `Virtual_Next` 中 item iterator 的 getter 改为 `dict->ItemAtIndex(index, isolate)`

  * 初始化类字典的 `PyDict::NewInstance()` 切换到 `PyDict::New(isolate)`

* 目的：

  * 补齐 views / iterators 这条“隐藏调用链”，避免 item 迭代器仍通过 helper 间接回退到旧接口。

### P4：接口风格统一收口

* 修改文件：

  * 全仓直接调用 `PyDict::NewInstance()` 的实际剩余文件

* 变更内容：

  * 将 `PyDict::NewInstance(...)` 全量替换为 `PyDict::New(isolate, ...)`

  * 如还有头文件注释 / 命名残留，统一收口

* 决策：

  * 本轮不保留兼容壳，不做双轨 API。

  * 本轮只统一 `PyDict`，不顺带把 `PyString::NewInstance` 一并升级，避免扩大议题。

### P5：验证与回归

* 修改文件：

  * `test/unittests/objects/py-dict-unittest.cc`

  * `test/unittests/interpreter/interpreter-dict-unittest.cc`

  * 视改动情况补充 `dict views` / `module` 相关解释器回归

* 变更内容：

  * 对象层优先覆盖：`Put / Remove / Get / GetTagged / GetKeyTuple / rehash`

  * 解释器层优先覆盖：`dict.get/pop/setdefault`、`keys/items/values` 迭代、模块导入中 dict 路径

* 验证顺序：

  1. 先用检索确认不存在 `PyDict::NewInstance(` 残留调用
  2. 再检索 `PyDict` 原语旧签名残留调用
  3. 获取 diagnostics，确认接口改造后无静态报错
  4. 在允许执行时再进行编译与定向测试

## Assumptions & Decisions

* 假设：你已经把 `PyDict::NewInstance` 的定义与声明改成 `PyDict::New`，后续只需清理调用点与相关引用。

* 决策：第四期不触碰 `HandleScope` 显式 isolate 化；即使 `HandleScope` 内部仍依赖 `Isolate::Current()`，也不作为本轮阻塞项。

* 决策：`ModuleUtils::IsPackageModule/GetPackagePathList` 先只做 `PyDict` API 透传适配；不在本轮升级其返回类型与错误语义。

* 决策：P2 与 P3 可以在同一实施批次完成，但代码提交建议按“主链透传”和“views 补齐”分两个逻辑块，便于审查。

* 决策：P4 只做 `PyDict` 创建入口统一，不外溢到 `PyString` 等其他对象类型。

## Verification

* 静态检查：

  * 检索 `PyDict::NewInstance\\(` 残留

  * 检索 `->ContainsKey\\(`、`->Get\\(`、`->GetTagged\\(`、`->Remove\\(`、
    `PyDict::Put\\(`、`->ItemAtIndex\\(`、`PyDict::GetKeyTuple\\(` 的旧签名残留

  * 使用 diagnostics 检查签名不匹配与缺失传参

* 运行验证（进入执行阶段后）：

  * 优先构建 `ut`

  * 定向跑 `PyDict` 对象层测试

  * 定向跑解释器层 `dict` / `module import` / `dict views` 相关测试

* 完成标准：

  * `PyDict` 原语直接调用链不再依赖隐式 current

  * `PyDict::NewInstance` 调用点清零

  * dict views / iterators 已切换到显式 isolate 形态

  * 定向测试通过且无新增 diagnostics

