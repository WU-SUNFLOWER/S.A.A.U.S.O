# 阶段 D 计划：旧 Handle 直构收口，且暂不动 HandleScope 构造函数

## Summary

本轮阶段 D 的目标是收口仍依赖旧式 `Handle(tagged)` / `handle(tagged)` 兼容层的调用与实现，但明确 **不改动** `HandleScope()` / `EscapableHandleScope()` 构造函数，也 **不改动** `HandleScope::AssertValidLocation(...)` 这组 API。\
执行完成后，代码库中的普通对象句柄提升路径将统一为显式 `Isolate*`，同时保留默认 scope 构造路径，作为下一轮继续清理 implicit-current 的边界。

## Current State Analysis

### 已确认的阶段 D 边界

* 用户已明确要求：

  * 本轮继续推进阶段 D。

  * **暂时不动** **`HandleScope`** **构造函数**。

  * **本轮先保持** **`HandleScope::AssertValidLocation`** **API 原样不动**。

  * 测试侧希望同步清理，而不是只保生产代码。

### 当前旧兼容链的实际结构

* [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)

  * 仍保留旧 `Handle(Tagged<T>)` 构造函数。

  * 仍保留旧 `handle(Tagged<T>)` 工具函数。

* [handle-base.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-base.h)

  * 仍保留 `HandleBase(Address object)`，其内部走 `HandleScope::CreateHandle(object)`。

* [handle-scopes.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scopes.h)

  * 仍保留默认 `HandleScope()` 与默认 `EscapableHandleScope()`。

  * 仍保留 `CreateHandle(Address)` 与 `AssertValidLocation(Address*)`。

* [handles.cc](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.cc)

  * 默认 `HandleScope()`、`CreateHandle(Address)`、`Extend()`、`AssertValidLocation(Address*)` 最终都回落到 `Isolate::Current()`。

### 当前仍需阶段 D 收口的“真实旧直构”点

* [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)

  * `Global<T>::Get(Isolate* isolate)` 仍返回 `Handle<T>(GetDirectionPtr())`，实质仍走旧 `Handle(Tagged)`。

* [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)

  * `Handle<T>::cast` 已改为走 `location()`，不再阻塞删除旧 `Handle(Tagged)`。

  * 但旧 `Handle(Tagged)` / `handle(tagged)` 实现本体仍在。

* 经过本轮前的搜索，`src/` 与 `test/` 中已基本没有“无 isolate 的 `handle(tagged)` / `Handle(tagged)` 调用点”，但仍存在大量 **显式 isolate 的** **`handle(tagged, isolate)`** 与 **`Handle(tagged, isolate)`** 调用，它们属于保留路径，不是删除对象。

### 当前与本轮无关、但仍保留的 implicit-current 范围

* `src/**` 中仍有 **149** 处默认 `HandleScope` / `EscapableHandleScope`。

* `test/**` 中仍有 **354** 处默认 `HandleScope` / `EscapableHandleScope`，分布在 **50** 个文件。

* [api-local-handle.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-local-handle.cc) 仍带有过渡期注释，说明内部 `HandleScope` 体系尚未完成显式 isolate 化。

* 由于本轮用户明确要求不动 `HandleScope` 构造函数，这部分默认 scope 使用只作为现状记录，不列入本轮实施范围。

## Proposed Changes

### 1. 收口 `Global<T>::Get` 的旧直构回落

**文件**

* [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)

**变更**

* 将 `Global<T>::Get(Isolate* isolate)` 从：

  * `return Handle<T>(GetDirectionPtr());`

* 改为显式 isolate 版本：

  * `return Handle<T>(GetDirectionPtr(), isolate);`

**原因**

* 这是当前“API 表面显式、内部仍回落旧直构”的唯一核心漏点。

* 只有先改掉这里，后续删除 `Handle(Tagged<T>)` 才不会误伤全局句柄读取路径。

### 2. 删除旧 `Handle(Tagged<T>)` 构造函数

**文件**

* [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)

**变更**

* 删除 `explicit Handle(Tagged<T> tagged)`。

* 保留 `Handle(Tagged<T> tagged, Isolate* isolate)`。

* 保留 `explicit Handle(Address* location)`，因为：

  * `Handle::cast`

  * `MaybeHandle::ToHandleChecked`

  * `MaybeHandle::ToHandle`

  * `EscapableHandleScope::Escape`

  * `Global<T>::Get`
    这几条链都合法依赖 slot/location 级别构造。

**原因**

* 阶段 D 的核心目标是删除“tagged 直接升 handle 但不显式传 isolate”的兼容入口。

* `Address* location` 构造不属于 implicit-current 路径，它只是包装已经存在的 slot，不应在本轮删除。

### 3. 删除旧 `handle(Tagged<T>)` 工具函数

**文件**

* [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)

**变更**

* 删除 `constexpr Handle<T> handle(Tagged<T> object)`。

* 保留 `constexpr Handle<T> handle(Tagged<T> object, Isolate* isolate)`。

**原因**

* 这与删除旧 `Handle(Tagged<T>)` 配套。

* 阻止任何代码继续通过 helper 函数隐式回落到 `Isolate::Current()`。

### 4. 删除旧 `HandleBase(Address object)` 构造函数

**文件**

* [handle-base.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-base.h)

**变更**

* 删除 `explicit constexpr HandleBase(Address object)`。

* 保留 `HandleBase(Address object, Isolate* isolate)` 与 `HandleBase(Address* location)`。

**原因**

* 旧 `Handle(Tagged<T>)` 的底层最终都汇聚到这里。

* 当旧 `Handle(Tagged<T>)` 与旧 `handle(tagged)` 删除后，`HandleBase(Address object)` 也应同步删除，避免留下可被误用的后门。

### 5. 同步修复受影响的生产代码与测试代码

**目标**

* 重新编译后，若出现任何仍依赖旧 `Handle(Tagged)` / `handle(tagged)` 的点，统一改成显式 isolate 版本。

**预期关注文件**

* 生产代码重点观察：

  * [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)

  * [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)

  * [handle-base.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-base.h)

* 测试代码重点观察：

  * [handle-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/handles/handle-unittest.cc)

  * [global-handle-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/handles/global-handle-unittest.cc)

  * [py-module-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/objects/py-module-unittest.cc)

  * 以及任何因头文件签名收紧而重新暴露出的单测残留点

**处理方式**

* 优先修复“真正因旧直构 API 删除而报错”的调用点。

* 不主动扩大到默认 `HandleScope` 使用面，不把本轮范围扩展为默认 scope 全量迁移。

### 6. 明确保留但不改动的兼容层

**本轮显式保留**

* [handle-scopes.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scopes.h)

  * `HandleScope()`

  * `EscapableHandleScope()`

  * `CreateHandle(Address ptr)`

  * `AssertValidLocation(Address* location)`

* [handles.cc](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.cc)

  * 与上述 API 对应的实现，包括 `Extend()`

**原因**

* 用户已明确要求本轮不动 `HandleScope` 构造函数。

* 用户额外要求本轮保持 `HandleScope::AssertValidLocation` API 原样不动。

* `CreateHandle(Address)` 与 `Extend()` 目前仍是默认 scope 路径的底层支撑；若本轮删除它们，会间接逼迫同时清理默认 scope，全量范围将明显扩大，不符合当前约束。

## Assumptions & Decisions

### 已锁定决策

* 本轮阶段 D 的完成定义是：

  * 删除旧 `Handle(Tagged<T>)`

  * 删除旧 `handle(Tagged<T>)`

  * 删除旧 `HandleBase(Address object)`

  * 修正 `Global<T>::Get(Isolate*)` 为显式 isolate 路径

  * 同步修复受影响的 src/test 调用点

* 本轮 **不** 删除：

  * `HandleScope()`

  * `EscapableHandleScope()`

  * `HandleScope::CreateHandle(Address)`

  * `HandleScope::AssertValidLocation(Address*)`

  * `HandleScope::Extend()`

### 关键判断

* 旧 `Handle(tagged)` 兼容层与默认 `HandleScope` 兼容层虽然共享同一个历史背景，但它们可以拆开分轮治理。

* 在当前边界下，最安全的策略是先彻底清掉“tagged -> handle 的隐式提升”，下一轮再治理“scope 默认构造 -> current isolate”。

* `MaybeHandle` 与 `Address* location` 体系本轮只做兼容，不做重构；它们不是阶段 D 的主要删除对象。

## Verification

实施时按以下顺序验证：

1. 先进行全仓搜索，确认 `src/` 与 `test/` 中不存在对已删除 API 的剩余引用。

   * 重点检查：

     * 旧 `handle(Tagged<T>)`

     * 旧 `Handle(Tagged<T>)`

     * 旧 `HandleBase(Address object)` 直接或间接使用
2. 运行语言诊断，确认头文件删改后没有新的签名错误。
3. 构建单测目标：

   * `ninja -C out/ut -j1 ut`

   * `ninja -C out/ut -j1 embedder_ut`
4. 运行测试：

   * `out/ut/ut.exe`

   * `out/ut/embedder_ut.exe`
5. 最后再次搜索确认：

   * 已删除 API 只剩声明变更后的显式 isolate 版本

   * 默认 `HandleScope` / `AssertValidLocation` 仍保留且未被误改

## Implementation Order

1. 修改 [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)
2. 修改 [handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h)
3. 修改 [handle-base.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-base.h)
4. 编译并根据报错回补 `src/` / `test/` 调用点
5. 跑 `ut` 与 `embedder_ut` 全量验证

