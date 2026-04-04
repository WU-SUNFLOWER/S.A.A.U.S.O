# S.A.A.U.S.O Phase 1 开工前 Checklist

本文档用于在 Phase 0 收官阶段，明确进入 **Phase 1：落地 OldSpace 基础能力** 之前的准备状态。

## 1. 目标

- 给出进入 Phase 1 前的最小前置条件
- 区分哪些事项必须完成，哪些可以并行推进
- 作为 Phase 0 收官与 Phase 1 开工之间的交接清单

## 2. 必须完成

### 2.1 写入路径收口已覆盖核心高风险对象

- `list / dict / function / type-object`
- `iterator / view / code-object`
- 对应 `Factory` 初始化路径已同步切到 setter 收口

判定标准：

- 新增对象字段写入不再继续扩大直接裸写面
- `Factory` 不再绕开已建立的 setter 约束

### 2.2 roots 审计已有正式书面产物

- roots 审计文档已存在：
- [Saauso-Roots-Audit-Phase0.md](file:///e:/MyProject/S.A.A.U.S.O/docs/generational-gc/Saauso-Roots-Audit-Phase0.md)

判定标准：

- 显式 roots、隐式 roots、MetaSpace 假设、TryCatch、模块缓存均已记录

### 2.3 TryCatch GC 安全性已收口

- `TryCatch` 捕获异常对象已升级为长期句柄持有
- 已有跨 inner scope 与 GC 压力的 embedder 回归测试

判定标准：

- `TryCatch` 不再依赖外层局部 handle 生命周期才能安全持有异常对象

### 2.4 StringTable 当前策略已冻结

- 当前继续保持“不开放 StringTable 根扫描”
- 但必须显式依赖并保护如下前提：
- `StringTable` 条目驻留 MetaSpace

判定标准：

- 测试已验证 `StringTable` 条目在 GC 后仍位于 MetaSpace 且可正常读取

## 3. 建议完成

### 3.1 地址语义审计文档已落地

- 文档：
- [Saauso-Address-Semantics-Audit-Phase0.md](file:///e:/MyProject/S.A.A.U.S.O/docs/generational-gc/Saauso-Address-Semantics-Audit-Phase0.md)

判定标准：

- 已明确 `repr` 与 `hash/id` 的语义分类
- 已冻结“不新增地址型语义点”的约束

### 3.2 roots 覆盖矩阵可继续细化

建议把以下持有方进一步整理为矩阵：

- `Isolate`
- `HandleScopeImplementer`
- `Interpreter`
- `ModuleManager`
- `ExceptionState`
- `BuiltinBootstrapper`
- `StringTable`
- `TryCatch`

### 3.3 剩余零散 holder 模式继续做尾扫

重点看：

- native cache / registry
- bootstrap 临时缓存
- 新增的自定义容器持有 `Tagged<T>` 的模式

## 4. 可与 Phase 1 并行推进

### 4.1 OldSpace 最小骨架设计

- `OldSpace::Setup`
- `OldSpace::Contains`
- `OldSpace::AllocateRaw`
- `Heap::InOldSpace()`

### 4.2 OldSpace 基础测试设计

- 地址归属判定
- 基础分配
- 分配失败行为

### 4.3 OldSpace 调试断言与 free block 预留设计

- 第一版可以不做完整 free list
- 但应预留后续 mark-sweep 的可扩展位置

## 5. 暂不要求在 Phase 1 前完成

- remembered set 正式启用
- `WRITE_BARRIER` 正式打开
- promotion
- major `mark-sweep`
- `mark-compact`

这些仍属于 Phase 1 之后或更后续阶段的内容。

## 6. 进入 Phase 1 的判断

当以下条件同时满足时，可以认为“已经足够进入 Phase 1”：

- 核心对象写入路径已经基本收口
- `TryCatch` 的 GC 安全性风险已被压住
- `StringTable` 当前策略已明确且有护栏测试
- roots 与地址语义至少已有正式文档，不再依赖口头约定

## 7. 一句话结论

Phase 1 的关键不是“所有 Phase 0 风险都被彻底消灭”，而是“最容易导致 OldSpace 设计返工的语义债已经被识别、冻结并有基本护栏”。  
在当前基础上，S.A.A.U.S.O 已经接近这个门槛，可以开始为 OldSpace 最小骨架实现做准备。
