# handle 显式 isolate 第一批调用面铺开计划

## Summary

* 目标：在你已完成阶段 2 的前提下，推进第一批“调用面铺开”，把全仓对旧式 `handle(obj)` / `Handle(tagged)` / `HandleBase(addr)` 的使用，优先改成显式 `Isolate*` 版本：

  * `handle(obj, isolate)`

  * `Handle(tagged, isolate)`

  * `HandleBase(addr, isolate)`

* 范围：本批以**调用点铺开**为主，不删除旧 API 定义；`HandleScope::CreateHandle(Address)`、`HandleScope::AssertValidLocation(Address*)` 等兼容桥仍保留，仅把调用面尽可能迁移到显式 isolate 形态。

* 交付标准：仓内主路径不再新增旧式 no-isolate handle 调用；既有调用点完成批量替换并通过全量 `ut` + `embedder_ut` 回归。

## Current State Analysis

### 1. 当前 handles 基建状态

* 目前新旧两套入口并存：

  * [handles.h:L20-L24](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h#L20-L24) 仍保留 `Handle(Tagged<T> tagged)`

  * [handles.h:L69-L88](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.h#L69-L88) 同时保留 `handle(Tagged<T> object)` 与 `handle(Tagged<T> object, Isolate* isolate)`

  * [handle-base.h:L32-L42](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-base.h#L32-L42) 同时保留 `HandleBase(Address object)` 与 `HandleBase(Address object, Isolate* isolate)`

* 兼容桥仍会落回 `Isolate::Current()`：

  * [handles.cc:L70-L72](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.cc#L70-L72) `CreateHandle(Address)` → `CreateHandle(Isolate::Current(), ...)`

  * [handles.cc:L101-L103](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.cc#L101-L103) `Extend()` → `Extend(Isolate::Current())`

  * [handles.cc:L118-L120](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handles.cc#L118-L120) `AssertValidLocation(Address*)` → `AssertValidLocation(Isolate::Current(), ...)`

### 2. 第一批调用面铺开的真实热点

* 语义检索显示旧式 no-isolate 调用不是零散残留，而是系统性存在：

  * `src/` 中 `handle(...)` 约 175 处 / 55 文件

  * `src/` 中 `Handle(...)` 直构约 34 处 / 17 文件

  * `test/` 中 `handle(...)` 约 321 处 / 37 文件

* 第一批最适合下手的是“显式 isolate 已经在手”的调用点，主要集中在：

  * `src/api/`

  * `src/runtime/`

  * `src/modules/`

  * `src/builtins/`

  * 一部分 `src/objects/` / `src/interpreter/` accessor

### 3. 代表性调用点与扩散器

* **宏扩散器**

  * [string-table.h:L20-L21](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h#L20-L21) 当前 `ST(x, isolate)` 仍展开成 `handle(ST_TAGGED(...))`，这是全仓最广的单点扩散器，必须纳入第一批。

* **API 层**

  * [api-support.cc:L18-L29](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-support.cc#L18-L29) 已拿到 `internal_isolate`，但仍调用旧式 `i::handle(...)`

  * [api-objects.cc:L12-L17](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc#L12-L17)、[api-objects.cc:L36-L46](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc#L36-L46) 也是典型“显式 isolate 已在手，调用仍旧式”

* **对象 / 栈帧 accessor**

  * [frame-object.cc:L57-L123](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.cc#L57-L123) 大量 `tagged -> handle` accessor，模式高度统一

  * [py-code-object.cc:L150-L191](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.cc#L150-L191) 同属典型 accessor 区

* **MaybeHandle 旁路**

  * [maybe-handles.h:L47-L48](file:///e:/MyProject/S.A.A.U.S.O/src/handles/maybe-handles.h#L47-L48) 目前 `MaybeHandle(Tagged<T>)` 仍复用旧式 `handle(tagged)`，第一批应同步补显式 isolate 通路，否则会形成新旧混用旁路。

### 4. 风险边界

* 第一批不适合直接删除旧 API，因为编译面太大；应先完成调用面铺开，再在后续批次删除旧入口。

* 第一批也不应同时处理：

  * `HandleScope::CreateHandle(Address)` 删除

  * `HandleScope::AssertValidLocation(Address*)` 删除

  * `Isolate::Init/TearDown()` 的 `Enter/Exit` 托底移除

* 否则很难分辨失败来源，也不利于全量回归。

## Proposed Changes

### 第 1 步：统一根接口与宏扩散器

* 修改 [string-table.h](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h)：

  * 将 `ST(x, isolate)` 从 `handle(ST_TAGGED(x, isolate))` 改为 `handle(ST_TAGGED(x, isolate), isolate)`。

* 修改 [maybe-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/maybe-handles.h)：

  * 为 `MaybeHandle` 增加显式 isolate 构造通路，避免其继续经由旧式 `handle(tagged)` 绕回 `Current()`。

* 原因：先收掉“宏与模板层的放大器”，后续目录替换才能真正有效，而不是一边铺开一边又从根上反向扩散旧 API。

### 第 2 步：优先迁移 `src/api/` 调用点

* 目标文件：

  * [api-support.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-support.cc)

  * [api-objects.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc)

  * [api-container.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-container.cc)

  * [api-context.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-context.cc)

  * [api-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-function.cc)

  * 以及 `src/api/` 下其余直接使用 `i::handle(...)` / `i::Handle(...)` 的文件

* 做法：

  * 已持有 `internal_isolate` / `i_isolate` 的地方统一改为显式 isolate 版本

  * 若是 `Handle(tagged)` 直构，改为 `Handle(tagged, isolate)`

* 原因：API 层 isolate 来源最清晰，替换最机械，且收益高。

### 第 3 步：迁移 `src/runtime/` / `src/modules/` / `src/builtins/`

* 目标文件：

  * `src/runtime/*.cc`

  * `src/modules/*.cc`

  * `src/builtins/*.cc`

* 做法：

  * 把所有 `handle(tagged)` 改为 `handle(tagged, isolate)` 或 `handle(tagged, isolate_)`

  * 把能直接改写的 `Handle(tagged)` 改为显式 isolate 直构

* 原因：这三层多数函数本就显式持有 `isolate` / `isolate_`，属于低阻力铺开区。

### 第 4 步：迁移 accessor / 栈帧 / 对象字段读取热点

* 重点文件：

  * [frame-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.cc)

  * [py-code-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.cc)

  * [py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.cc)

  * [py-dict-views.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.cc)

  * 以及其它类似 `field_tagged -> handle(...)` accessor 密集文件

* 做法：

  * 若对象实例天然已知 isolate，应复用已有显式 isolate 形参

  * 若当前 accessor 本身没有 isolate，但 owner 类型/调用链可以自然补入 isolate，则在本批中一并补函数签名

* 原因：这批文件模式统一、命中密集，是第一批的重要收口收益来源。

### 第 5 步：补齐 `test/` 跟随改造

* 目标文件：

  * `test/unittests/**`

  * `test/embedder/**`

* 做法：

  * 将测试中旧式 `handle(...)` / `Handle(...)` 用法统一替换为显式 isolate 版本

  * 测试夹具普遍已有 `isolate_` 或 `isolate`，替换应以机械方式完成

* 原因：用户要求“全仓铺开”，因此测试不应留到后面；但执行顺序上应放在 `src/` 主体收敛之后，作为跟随修复批次。

### 第 6 步：静态清点并保留兼容桥

* 本批结束时：

  * 保留旧 API 定义与 TODO，不在本批删除

  * 用静态检索确认仓内业务/测试调用面已基本收敛到显式 isolate 版本

* 原因：第一批目标是“铺开调用面”，不是“立即断根”；删除旧 API 是下一批任务。

## Assumptions & Decisions

* 决策 1：第一批只铺开 `handle(obj, isolate)` / `Handle(tagged, isolate)` / `HandleBase(addr, isolate)` 的**调用面**，不在本批删除兼容桥。

* 决策 2：本批把 `string-table.h` 与 `maybe-handles.h` 视为“根扩散器”，必须纳入改造范围。

* 决策 3：本批包含 `src/` 与 `test/`；但执行顺序上先 `src/`，后 `test/`，以减少编译波动。

* 决策 4：若某个 accessor/helper 当前没有 isolate，但其 owner/调用链可自然补入 isolate，则允许在本批局部补签名；若需要大面积架构重整，则留到后续批次。

* 决策 5：本批不处理 `HandleScope::CreateHandle(Address)` / `AssertValidLocation(Address*)` 的删除，也不处理 `Isolate::Init/TearDown()` 的 `Enter/Exit` 托底移除。

## Verification

* 静态检查：

  * 检索 `src/` 与 `test/` 中旧式 `handle(...)` 调用残留

  * 检索 `Handle(tagged)` 直构残留

  * 检索 `HandleBase(addr)` 旧构造链是否仍在新增路径中被使用

* 编译验证：

  * `gn gen out/ut --args="is_debug=true"`

  * `ninja -C out/ut ut`

  * `ninja -C out/ut embedder_ut`

* 回归验证：

  * `out/ut/ut.exe`

  * `out/ut/embedder_ut.exe`

* 通过标准：

  * 构建通过

  * `ut` 通过

  * `embedder_ut` 通过

  * 第一批目标调用面已基本显式 isolate 化

  * 未引入新的 `Isolate::Current()` 调用

