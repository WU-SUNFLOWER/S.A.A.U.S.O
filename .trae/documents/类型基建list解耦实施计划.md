# 类型基建 List 解耦实施计划

## 1. 目标与原则

* 目标：将 `PY_TYPE_LIST/PY_TYPE_IN_HEAP_LIST` 从 `object-checkers.h` 继续解耦，沉淀为可复用的“基建类型清单”头文件，供 VM 上层与子系统统一复用。

* 原则：

  * 不改变现有 `IsPyXxx/IsPyXxxExact` 语义与对外函数名。

  * 不改动 `ISOLATE_KLASS_LIST` 当前职责（仅 Isolate 初始化/访问器/字段生成）。

  * 先做“单一事实来源（Single Source of Truth）”的类型清单沉淀，再让 checker 与其他模块引用该清单。

## 2. 设计方案

### 2.1 新增独立基建清单文件

* 新建：`src/objects/object-type-list.h`

* 只承载“对象类型清单宏”，不放任何 checker 声明/实现：

  * `PY_TYPE_IN_HEAP_LIST(V)`

  * `PY_TYPE_LIST(V)`

* 宏内容保持与当前语义一致：

  * `PY_TYPE_IN_HEAP_LIST` 仅包含堆对象类型；

  * `PY_TYPE_LIST` = `PySmi + PY_TYPE_IN_HEAP_LIST`。

### 2.2 职责边界调整

* `object-checkers.h`：

  * 移除本地 `PY_TYPE_*` 宏定义；

  * 改为 `#include "src/objects/object-type-list.h"`；

  * 继续基于 `PY_TYPE_LIST` 生成前置声明与 checker 声明。

* `object-checkers.cc`：

  * 不直接依赖 `PY_TYPE_*` 宏定义文件（经 `object-checkers.h` 间接可见即可）；

  * 保持实现逻辑不变。

* `py-object.h`：

  * 继续仅包含 `object-checkers.h`，不额外感知 `object-type-list.h`。

### 2.3 与 `ISOLATE_KLASS_LIST` 的关系

* 本次不合并 `ISOLATE_KLASS_LIST` 与 `PY_TYPE_LIST`，原因：

  * `ISOLATE_KLASS_LIST` 是三元组（Type/Klass/slot）并驱动初始化链路，风险高、影响面大；

  * `PY_TYPE_LIST` 主要是类型枚举与 checker 声明生成，语义层级不同。

* 但预留后续演进方向：

  * 在文档中明确两者是“同域不同职责”的基建 list；

  * 后续若要进一步统一，可通过新增中间元数据表（X-macro 表）派生两个 list，而不是直接互相替代。

## 3. 具体实施步骤

### Step A：创建基建清单头

1. 新增 `src/objects/object-type-list.h`，定义并导出 `PY_TYPE_IN_HEAP_LIST/PY_TYPE_LIST`。
2. 文件仅包含宏定义与 include guard，不引入其他头文件，避免依赖污染。

### Step B：改造 checker 头

1. 修改 `src/objects/object-checkers.h`：

   * 删除本地 `PY_TYPE_*` 宏定义；

   * include 新的 `object-type-list.h`；

   * 保持 `DECLARE_PY_TYPE/DECLARE_PY_CHECKER` 展开逻辑不变。
2. 确认 `object-checkers.h` 对外可见 API 无变化。

### Step C：构建系统与引用检查

1. 更新 `BUILD.gn` sources：

   * 新增 `//src/objects/object-type-list.h`。
2. 全仓静态扫描：

   * `PY_TYPE_LIST/PY_TYPE_IN_HEAP_LIST` 仅在新基建头定义；

   * 使用方通过 include 引入，不再重复定义。

### Step D：文档同步

1. 更新 `AGENTS.md`：

   * 标注 `PY_TYPE_*` 的新归属路径（`src/objects/object-type-list.h`）；

   * 说明 `ISOLATE_KLASS_LIST` 与 `PY_TYPE_LIST` 的职责边界。

### Step E：验证与回归

1. 运行 `GetDiagnostics`，确保无新增诊断错误。
2. 构建验证：`ninja -C out\\ut ut`。
3. 回归验证：`out\\ut\\ut.exe` 全量单测通过。
4. 对比关键语义点：

   * like/exact checker 单测继续通过；

   * 依赖 checker 的边界行为（exec/DICT\_MERGE/CALL\_FUNCTION\_EX 等）不回归。

## 4. 风险与应对

* 风险1：宏重定义或重复来源导致编译冲突。

  * 应对：确保 `PY_TYPE_*` 只在 `object-type-list.h` 定义；全仓 grep 校验。

* 风险2：头文件包含顺序变化引发前置声明缺失。

  * 应对：在 `object-checkers.h` 维持 `DECLARE_PY_TYPE` 展开，保留必要前置声明能力。

* 风险3：文档与代码路径不一致。

  * 应对：改造完成后同步修订 AGENTS 对应章节并复核引用位置。

## 5. 验收标准

* `PY_TYPE_LIST/PY_TYPE_IN_HEAP_LIST` 已从 `object-checkers.h` 完全迁出到 `object-type-list.h`。

* checker API（函数名、语义、调用方式）保持兼容。

* `ISOLATE_KLASS_LIST` 维持现状且无行为变化。

* 构建通过、全量 UT 通过、诊断无新增错误。

