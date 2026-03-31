# HandleScope / EscapableHandleScope 旧式构造函数拔除分阶段计划

## 1. 目标与结论

* 目标：在不破坏现有稳定性的前提下，分阶段完成以下两项拔除：

  * `HandleScope()`（无参旧构造）

  * `EscapableHandleScope()`（依赖基类无参构造的旧构造）

* 结论：当前仓库已具备启动拔除工程的成熟度，但需要先做“调用面收口 + 阶段闸门 + 回归矩阵”三步，避免一次性硬删造成大面积回归噪音。

## 2. 现状快照（调研基线）

### 2.1 构造函数与 Current 依赖

* 旧式构造与新式构造并存：`HandleScope()` 与 `explicit HandleScope(Isolate*)`、`EscapableHandleScope()` 与 `explicit EscapableHandleScope(Isolate*)` 同时存在（`src/handles/handle-scopes.h`）。

* `HandleScope()` 实现仍直接走 `Isolate::Current()`（`src/handles/handles.cc`）。

* `Isolate::Init/TearDown` 仍使用 `Enter/Exit + HandleScope scope;`，并明确标注了后续移除 Current 依赖的 TODO（`src/execution/isolate.cc`）。

### 2.2 调用面数量（语法形态统计）

* `HandleScope 变量声明无参构造`：

  * `src/*.cc`：32 处

  * `test/*.cc`：341 处

* `EscapableHandleScope 变量声明无参构造`：

  * `src/*.cc`：117 处（高度集中在 runtime / builtins / heap / api）

* `HandleScope 显式 isolate 构造`：48 处（主要在 embedder/test/samples）。

### 2.3 关键边界

* API 桥接侧已有显式 isolate 透传能力（如 `src/api/api-local-handle.cc`、`src/api/api-support.cc`），可以作为第一批低风险收口区。

* 句柄底层仍存在 `CreateHandle(Address)`、`AssertValidLocation(Address*)` 等 Current 兼容入口，建议与“构造函数拔除”解耦，避免同批风险叠加。

## 3. 分阶段实施计划（每阶段可独立回归）

## 阶段 A：建立基线与闸门（不删 API）

### A.1 实施内容

* 固化当前基线统计脚本（按目录统计无参构造命中数）。

* 增加“防回退闸门”：

  * 至少先对 `src/api/**`、`src/runtime/**`、`src/modules/**`、`src/builtins/**` 启用 no-new-usage 检查。

  * 约束目标：这些目录中不得新增 `HandleScope xxx;` 与 `EscapableHandleScope xxx;`。

### A.2 阶段验收

* 静态验收：目录级 grep 基线文件可重复生成，且与主干一致。

* 回归验收：

  * 构建通过：`ut`、`embedder_ut`

  * 运行通过：`ut`、`embedder_ut`

***

## 阶段 B：低风险调用面收口（API / 桥接 / Embedder）

### B.1 实施内容

* 优先迁移已天然持有 isolate 的区域：

  * `src/api/**`（`HandleScope scope;` / `EscapableHandleScope scope;` -> 显式 isolate 构造）

  * embedder 相关调用链中仍残留的无参构造

* 同步调整局部 helper/impl 的构造透传，避免在 API 适配层重新引入无参构造。

### B.2 阶段验收

* 静态验收：

  * `src/api/**/*.cc` 中无参构造命中为 0。

  * 关键桥接文件（如 `api-local-handle.cc`）仅保留显式 isolate 构造路径。

* 回归验收：

  * `embedder_ut` 全量通过

  * `ut` 全量通过

***

## 阶段 C：生产主链收口（runtime / builtins / modules / objects / heap）

### C.1 实施内容

* 分目录推进无参构造替换，统一显式传入 `isolate` / `isolate_` / `this`（按上下文）。

* 优先顺序建议：

  1. `src/runtime/**`
  2. `src/modules/**` + `src/builtins/**`
  3. `src/objects/**` + `src/heap/**` + `src/interpreter/**`

* 对 isolate 来源不清晰的少量点位，先做“就近透传”再替换，不做一次性大重构。

### C.2 阶段验收

* 静态验收：

  * `src/**/*.cc` 中 `HandleScope xxx;` 命中清零（仅允许类型声明/非调用语法残留）。

  * `src/**/*.cc` 中 `EscapableHandleScope xxx;` 命中清零（同上）。

* 回归验收：

  * `ut` 全量通过

  * `embedder_ut` 全量通过

***

## 阶段 D：测试调用面收口（test 全量）

### D.1 实施内容

* 将 `test/unittests/**` 与 `test/embedder/**` 中无参构造统一替换为显式 isolate。

* 统一测试夹具中的 isolate 取值方式，减少测试体内重复样板。

### D.2 阶段验收

* 静态验收：`test/**/*.cc` 的无参构造命中清零。

* 回归验收：

  * `ut` 全量通过

  * `embedder_ut` 全量通过

  * 句柄相关重点用例（handles/gc/embedder-isolate）无新增波动。

***

## 阶段 E：拔除旧构造定义（真正断根）

### E.1 实施内容

* 删除声明与实现：

  * `HandleScope();`

  * `EscapableHandleScope() = default;`

  * `HandleScope::HandleScope() : HandleScope(Isolate::Current()) {}`

* 处理初始化链路残留：

  * `Isolate::Init/TearDown` 中的 `HandleScope scope;` 改为显式 `HandleScope scope(this);`

* 清理对应 TODO 文案，更新规范：HandleScope 家族必须显式 isolate。

### E.2 阶段验收

* 编译期验收：全仓无调用点可再匹配旧式无参构造。

* 回归验收：

  * `ut` 全量通过

  * `embedder_ut` 全量通过

* 语义验收：多线程/未 Enter isolate 的负向契约测试行为不劣化。

## 4. 回归矩阵（建议每阶段固定执行）

* 最小必跑：

  * `ut`

  * `embedder_ut`

* 重点关注子域：

  * handles：scope/escape/nesting/death tests

  * heap/gc：跨 scope 生命周期与回收稳定性

  * embedder-isolate：线程与 isolate 绑定契约

* 建议策略：

  * 每阶段先跑“受影响目录相关目标 + 重点子域”

  * 阶段收尾再跑全量 `ut + embedder_ut`

## 5. 风险与缓解

* 风险 1：隔离实例透传不完整，导致局部点位编译失败或行为漂移。

  * 缓解：按目录小步提交；每批只做一种形态替换；先 `src/api` 再核心路径。

* 风险 2：Current 兼容入口与构造函数拔除并行，失败根因难定位。

  * 缓解：本计划仅聚焦“构造函数无参入口”拔除；`CreateHandle(Address)` 等兼容 API 延后独立治理。

* 风险 3：测试改造量大、噪音高。

  * 缓解：测试收口放到阶段 D，确保生产主链先稳定。

## 6. 完成定义（DoD）

* `src/handles/handle-scopes.h` 不再声明无参旧构造。

* `src/handles/handles.cc` 不再实现 `HandleScope()` 的 Current 转发。

* `src/**/*.cc` 与 `test/**/*.cc` 中不再出现无参构造调用形态。

* `ut` 与 `embedder_ut` 全量通过。

