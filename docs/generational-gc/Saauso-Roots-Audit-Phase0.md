# S.A.A.U.S.O Phase 0 Roots 审计

本文档用于固化 Phase 0 的 roots 审计结果，重点回答四个问题：

- 当前 GC roots 的显式入口有哪些
- 当前依赖哪些隐式 root 或工程假设
- 哪些 native 持有路径需要在后续老生代 GC 中重点关注
- 哪些位置虽然当前可用，但会在 generational / major GC 中成为风险点

## 1. 总体结论

- 当前显式 root tracing 总入口集中在 `Heap::IterateRoots()`。
- 当前 minor GC 可工作，但正确性仍明显依赖 `MetaSpace` 永久对象不移动这一假设。
- 当前未发现 `std::vector/std::map/std::unordered_map` 直接长期持有 `Tagged<T>` 的主路径实现；真正需要关注的是项目自定义 `Vector` 容器以及若干 bootstrap 缓存。
- `TryCatch`、`BuiltinBootstrapper`、模块注册表、解释器栈帧与 handle blocks 都属于必须长期纳入审计模型的“隐式 roots 持有方”。

## 2. 显式 Roots 总入口

当前 GC roots 总入口位于：

- [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc)

当前扫描顺序可以归纳为：

1. `Isolate`
2. `klass_list_`
3. `HandleScopeImplementer`
4. `Interpreter`
5. `ModuleManager`
6. `ExceptionState`

这意味着当前 major GC 演进的第一原则仍然成立：

- **后续任何新增的长期持有方，都必须被接到 `Heap::IterateRoots()` 或其下游 `Iterate()` 链路中。**

## 3. Roots 覆盖矩阵

### 3.1 `Isolate`

- 关键文件：
- [isolate.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.h)
- [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc)

- 当前覆盖：
- `Isolate::Iterate()` 当前直接暴露 `builtins_`

- 风险：
- `py_none_object_ / py_true_object_ / py_false_object_` 当前依赖 MetaSpace 永久对象假设
- `try_catch_top_` 不是 GC root slot，而是 API 层异常链表头，需要单独审计其对象持有语义

- 审计结论：
- `Isolate` 目前不是“所有长期对象的单点真相”，而是“部分直接暴露 + 其他对象由外层单独遍历”的模式
- 这一点在 future major GC 中要持续警惕

### 3.2 `klass_list_`

- 关键文件：
- [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc)
- [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)

- 当前覆盖：
- `Heap` 直接遍历 `klass_list_`
- `Klass::Iterate()` 进一步遍历 `name_ / type_object_ / klass_properties_ / supers_ / mro_`

- 风险：
- `Klass` 属于天然长寿对象
- 一旦 `type_object_` 或 `mro_` 内部对象未来进入普通堆，这条链就会成为老生代根的核心组成部分

### 3.3 `HandleScopeImplementer` 与 `GlobalHandles`

- 关键文件：
- [handle-scope-implementer.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scope-implementer.h)
- [handle-scope-implementer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scope-implementer.cc)
- [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)

- 当前覆盖：
- local handle blocks
- global handle blocks

- 风险：
- API 层和 bootstrap 期大量对象都依赖 handle slot 保活
- 若 future major GC 引入更复杂的 safepoint 规则，handle blocks 仍然必须保持为最可信赖的根来源之一

### 3.4 `Interpreter` 与 `FrameObject`

- 关键文件：
- [interpreter.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.h)
- [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc)
- [frame-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.h)
- [frame-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.cc)

- 当前覆盖：
- `ret_value_`
- `kwarg_keys_`
- `caught_exception_`
- `current_frame_`
- frame 内部的 `stack_ / consts_ / names_ / locals_ / localsplus_ / globals_ / code_object_ / func_`

- 风险：
- 解释器路径是当前最重要的 native roots 聚合点之一
- 一旦 frame 中某个字段未来改成新的 backing store 或 side structure，必须同步补 `Iterate()`

### 3.5 `ModuleManager` 与模块缓存

- 关键文件：
- [module-manager.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.h)
- [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)
- [builtin-module-registry.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-registry.h)
- [builtin-module-registry.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-registry.cc)

- 当前覆盖：
- `sys.modules`
- `sys.path`
- builtin module registry

- 风险：
- 这类 registry/cache 非常容易在未来扩展时新增字段，但忘记同步到 `Iterate()`
- 属于“现在能用，但未来很容易被新人漏掉”的高风险模式

### 3.6 `ExceptionState`

- 关键文件：
- [exception-state.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-state.h)
- [exception-state.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-state.cc)

- 当前覆盖：
- `pending_exception_`

- 风险：
- 这只能覆盖“当前挂在 ExceptionState 上”的异常对象
- 不能据此推断“整个异常链路已经完整收口”

## 4. 隐式 Roots 与工程假设

### 4.1 `BuiltinBootstrapper`

- 关键文件：
- [builtin-bootstrapper.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.h)
- [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc)
- [templates.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/templates.h)

- 当前持有方式：
- bootstrap 期间通过 `Global<PyDict> builtins_` 暂存 builtins 字典
- native function 模板参数通过 `FunctionTemplateInfo` 内的 `Global<>` 保活

- 风险：
- 这条链目前可用，但依赖“bootstrap 期对象都通过 Global/Handle 托管”
- 若未来引入新的 bootstrap 中间缓存，必须同步进入 roots 模型

### 4.2 `StringTable`

- 关键文件：
- [string-table.h](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h)
- [string-table.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.cc)

- 当前状态：
- `StringTable::Iterate()` 已存在
- 但 `Heap::IterateRoots()` 当前没有启用这条路径

- 当前依赖的假设：
- 字符串常量主要驻留在 MetaSpace，因此 minor GC 不需要主动遍历它

- 风险：
- 这是当前最典型的“未来 major GC 会出问题”的入口之一
- 只要字符串对象分配策略变化，这里就必须立刻打开显式 root tracing

### 4.3 `TryCatch` 与异常展开链路

- 关键文件：
- [api-exception.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-exception.cc)
- [api-exception-support.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-exception-support.cc)
- [saauso-exception.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso-exception.h)

- 当前状态：
- `TryCatch` 自身通过 `try_catch_top_` 形成链表
- 捕获到的异常值保存为 `Local<Value>`

- 风险：
- `Local<Value>` 本质是 handle slot 地址，不是 `Global`
- 它的安全性依赖外层 handle scope 生命周期，而不是 `Heap::IterateRoots()` 的直接成员扫描
- 这在 future major GC / API 边界 / 跨层异常传播中必须持续复核

## 5. 容器持有审计

### 5.1 标准库容器

- 当前结论：
- 未发现 `std::vector<std::...Tagged<T>>`、`std::map<..., Tagged<T>>`、`std::unordered_map<..., Tagged<T>>` 这类长期持有实现

- Phase 0 审计结论：
- 当前“标准库容器持有 `Tagged<T>`”不是主要风险来源
- 但后续若新增此类用法，必须同步纳入 roots 文档

### 5.2 自定义 `Vector`

- 当前需要重点关注的自定义容器：
- `klass_list_`
- `remembered_set_`
- `BuiltinModuleRegistry::entries_`

- 风险：
- 这些并不属于标准 handle/root 抽象
- 必须依赖外层显式 `Iterate()` 或循环访问

## 6. 风险清单

### 6.1 高风险

- `StringTable` 仍依赖 MetaSpace 假设
- `TryCatch` 捕获值不是 `Global`
- future remembered set 启用后，old-to-young 引用模型还未形成完整闭环

### 6.2 中风险

- `BuiltinBootstrapper` 若新增临时缓存，可能绕开当前 roots 模型
- `ModuleManager` / builtin registry / 各类 native cache 后续扩展时容易漏 `Iterate()`

### 6.3 低风险

- 当前未发现标准库容器长期持有 `Tagged<T>` 的主路径
- 但这不应被视为未来可放松的规则

## 7. 对 Phase 1 的输入要求

在进入 `OldSpace + promotion + mark-sweep` 之前，至少应满足：

- 明确 `StringTable` 是否继续依赖 MetaSpace，还是纳入普通 root tracing
- 明确 `TryCatch` 持有异常对象在 future major GC 下的生命周期保证
- 新增任何 native cache / registry / container 持有 `Tagged<T>` 时，必须同步补 `Iterate()` 或文档化说明
- roots 文档必须随架构演进持续更新，而不是一次性产物

## 8. 一句话结论

当前 roots 体系对 minor GC 已够用，但对 future generational / major GC 而言仍带有明显的“依赖工程假设”色彩。  
Phase 0 的目标不是立刻重写整套 root 系统，而是先把这些假设显式记录出来，并把容易新增遗漏的持有方纳入持续审计范围。
