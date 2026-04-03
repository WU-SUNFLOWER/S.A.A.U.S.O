# S.A.A.U.S.O Phase 0 详细任务拆解

本文档用于把老生代 GC 计划中的 Phase 0 拆解成可执行的工程任务，明确说明：

- Phase 0 的目标是什么
- 预计会改哪些文件
- 每类文件为什么要改
- 每一步的主要风险点是什么
- 哪些内容是 Phase 0 完成标准，哪些不属于 Phase 0

本文档吸收了以下既有结论：

- [Saauso-Old-Generation-GC-Development-Plan.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Old-Generation-GC-Development-Plan.md)
- [Saauso-Old-Generation-GC-Review.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Old-Generation-GC-Review.md)

## 1. Phase 0 的定位

Phase 0 不是直接实现 `OldSpace`、promotion 或 major GC。  
Phase 0 的职责是先把后续 GC 正确性所依赖的工程护栏建立起来，避免后续 Phase 1~Phase 4 在错误基础上叠加复杂度。

结合技术评审报告，Phase 0 的核心目标收敛为四件事：

1. 冻结关键语言与运行时语义前提。
2. 冻结 GC 元数据方向，优先采用 side metadata / bitmap。
3. 把对象引用字段的写入路径收口到 `private + getter + setter + write barrier`。
4. 对 roots、地址语义、测试护栏做专项审计并形成清单。

此外，Gemini 的审查意见已经补充确认以下执行约束：

- `factory.cc` 不能被完全后置，必须与第一批对象字段私有化同步改造。
- 所有运行时中使用 `std::vector<Tagged<T>>`、`std::unordered_map<..., Tagged<T>>` 一类 C++ 标准库容器持有堆对象引用的路径，都必须纳入 roots 审计范围。
- `BuiltinBootstrapper`、初始化期静态缓存以及类似“全局单例持有 `Tagged<T>`”的代码路径，都必须纳入隐式 roots 审计。
- `TryCatch`、异常展开与 C++ 临时异常对象持有链路，不能只假设由 `ExceptionState` 单点覆盖。
- write barrier 的慢路径实现应尽量放在 `.cc` 文件中，避免复杂屏障逻辑在头文件中大面积内联扩散。

## 2. Phase 0 完成后必须得到的结果

- 项目层面明确不支持 `__del__`，避免 finalizer / resurrection 复杂度。
- 项目层面明确未来 `id()` 不依赖对象物理地址。
- 项目层面明确 future moving GC 不受当前 `repr/hash` 地址语义绑定的长期约束。
- 对象引用字段的“直接裸写”明显减少，并建立统一 setter 规范。
- 后续 write barrier 启用前，关键对象类型已经具备可收口的写入结构。
- major roots、地址语义、测试缺口形成文档化清单。

## 3. Phase 0 不做什么

- 不在本阶段实现 `OldSpace` 分配器。
- 不在本阶段正式启用 remembered set。
- 不在本阶段正式开启 `WRITE_BARRIER` 宏。
- 不在本阶段实现 promotion。
- 不在本阶段实现 major `mark-sweep` 或 `mark-compact`。

换句话说，Phase 0 的目标是“把地基浇实”，而不是“把整栋楼先搭起来”。

## 4. 总体任务分组

Phase 0 建议分为五个工作流：

- **W0：架构决议冻结**
- **W1：写入路径收口与访问控制重构**
- **W2：GC roots 审计**
- **W3：地址语义与未来 Identifier 审计**
- **W4：测试护栏补齐**

下面逐项展开。

## 5. W0：架构决议冻结

### 5.1 目标

- 把技术评审中已经达成共识的三项关键决议正式固化：
- `id()` 未来不依赖对象物理地址
- `__del__` 不进入该 VM 的主路线
- 对象引用字段必须走 `private + setter`

### 5.2 预计修改文件

- [Saauso-Old-Generation-GC-Development-Plan.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Old-Generation-GC-Development-Plan.md)
- [Saauso-Old-Generation-GC-Review.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Old-Generation-GC-Review.md)
- [GC-and-Heap.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/GC-and-Heap.md)
- [Object-System.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Object-System.md)
- [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)

### 5.3 为什么改这些文件

- `Development-Plan` 是总计划基线，必须吸收评审后的方向修正。
- `Review` 是外部技术审查意见来源，便于后续开发时追溯“为什么这么做”。
- `GC-and-Heap` 需要更新为“当前状态 + 已冻结演进方向”。
- `Object-System` 与 `Engineering-Constraints` 需要补充对象字段封装、禁止裸写、future identifier 语义等工程约束。

### 5.4 风险点

- 只在计划文档里写约束，但不落到工程规范里，会导致后续实现时出现“知道原则但无执行标准”的问题。
- 如果 side metadata 方案不在 Phase 0 直接拍板，后续 `MarkWord` 改造会持续摇摆。
- 如果 `id()`、`__del__`、side metadata、setter-only 写入这几项决议没有同步落到架构与约束文档，后续实现中极易再次出现“临时妥协”。

## 6. W1：写入路径收口与访问控制重构

### 6.1 目标

- 找出所有“对象引用字段”的写入点。
- 将这些字段逐步改为 `private`。
- 为这些字段提供 getter / barrier-aware setter。
- 为未来启用 `WRITE_BARRIER` 做结构性准备。

### 6.2 改造原则

- 所有 `Tagged<T>` 引用字段，原则上都不允许外部直接赋值。
- 构造期如果必须裸写，必须限制在“对象未发布、且受 `DisallowHeapAllocation` 保护”的窗口内。
- 一旦对象对外可见，后续写入必须走 setter。
- setter 的函数签名和命名风格应在同类对象中保持一致。
- write barrier 的慢路径、记录逻辑与复杂判定应尽量收口在 `.cc` 中，不要把未来复杂屏障逻辑全部塞进头文件 inline 接口里。

### 6.3 第一批优先修改文件

#### A. 堆与屏障总入口

- [heap.h](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.h)
- [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc)

**为什么改**

- 这里定义了 `WRITE_BARRIER`、`RecordWrite()`、remembered set 入口，是未来所有 setter 的统一后端。
- 即便 Phase 0 不正式打开屏障，也必须先把未来调用约定定稳。

**风险点**

- 如果过早启用 `WRITE_BARRIER`，而对象字段还没收口，会导致大量漏记和难以定位的 GC bug。
- 如果不先稳定 setter 与 barrier 的接口形态，后续对象层改造会重复返工。
- 如果屏障慢路径过度内联进头文件，后续会放大 include 依赖、编译体积与调试复杂度。

#### B. 基础容器模板文件

- [fixed-array.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.h)
- [fixed-array.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.cc)
- [py-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.h)
- [py-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.cc)
- [cell.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/cell.h)
- [cell.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/cell.cc)
- [py-tuple.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple.h)
- [py-tuple.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple.cc)

**为什么改**

- 这些文件里已经存在较成熟的 setter 模式，是后续统一风格的模板来源。
- `FixedArray::Set()` 未来会成为大量容器写入的安全路径基础。
- `PyObject::SetProperties()`、`Cell::set_ref()`、`PyTuple::Set()` 可以沉淀出统一规范。

**风险点**

- 如果模板本身的接口风格不统一，后续批量治理对象类型时会出现命名与使用方式分裂。

#### C. 第一批高风险对象

- [py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h)
- [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc)
- [py-dict.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.h)
- [py-dict.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.cc)
- [py-function.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.h)
- [py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.cc)
- [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h)
- [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)
- [py-type-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.h)
- [py-type-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.cc)

**为什么改**

- 这些对象寿命长、引用关系复杂、未来极可能进入老生代。
- 它们持有的字段大多是“对象引用字段”，最容易在 promotion 与 old-to-new 写入场景下触发漏屏障问题。
- `list` / `dict` / `function` / `klass` / `type-object` 几乎覆盖了运行时的高频长期对象。

**风险点**

- 这些对象文件被大量调用，一旦字段私有化，会触发广泛编译错误。
- `friend class Factory` 可能使“字段改 private”形式上完成，但工程约束仍然被绕过，需要同步限制 factory 写入窗口。
- `dict` 和 `list` 的元素写入有一部分已通过 `FixedArray::Set()` 安全，但“容器本身持有 backing store”的字段更新仍是高风险点，容易产生假安全感。

#### D. 第二批模式化对象

- [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h)
- [py-code-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.cc)
- [py-dict-views.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.h)
- [py-dict-views.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.cc)
- [method-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/method-object.h)
- [method-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/method-object.cc)
- [py-list-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.h)
- [py-list-iterator.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.cc)
- [py-tuple-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.h)
- [py-tuple-iterator.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.cc)

**为什么改**

- 这些对象模式比较统一，通常持有 `owner_`、`func_`、`code_`、`tuple_`、`list_` 一类引用字段，适合在第一批规范稳定后批量治理。

**风险点**

- 这批文件数量多，若在第一批规范未稳定前同时推进，容易造成 API 风格反复调整。

#### E. 构造入口与初始化路径

- [factory.h](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.h)
- [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc)

**为什么改**

- 工厂是对象创建主入口，大量字段初始化发生在这里。
- 如果不明确“哪些写入属于初始化窗口，哪些写入必须走 setter”，后续写屏障规则无法落地。

**风险点**

- Factory 是全仓库级高耦合点，改动稍大就可能引发大面积编译失败。
- 如果把所有初始化都强行改成走 setter，可能与当前 `DisallowHeapAllocation` 初始化策略冲突，需要明确豁免边界。

### 6.4 W1 建议执行顺序

1. 先把模板文件的 setter 规范写清楚。
2. 再处理 `list / dict / function / klass / type-object`，并同步修改 `factory.cc` 中对应对象的初始化路径。
3. 再处理 iterator / view / method / code-object，并继续同步修正相关 factory 分配入口。
4. 最后回到 `factory`，收束剩余初始化窗口规则，而不是把 `factory.cc` 整体留到最后。

## 7. W2：GC Roots 审计

### 7.1 目标

- 确认当前 GC roots 是否已经形成单点收口。
- 找出 major GC 未来可能遗漏的 native roots。
- 区分“当前因为都在 MetaSpace 而安全”和“真正被 root tracing 覆盖”的对象。

### 7.2 预计重点审计文件

- [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc)
- [isolate.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.h)
- [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc)
- [handle-scope-implementer.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scope-implementer.h)
- [handle-scope-implementer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/handles/handle-scope-implementer.cc)
- [global-handles.h](file:///e:/MyProject/S.A.A.U.S.O/src/handles/global-handles.h)
- [interpreter.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.h)
- [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc)
- [frame-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.h)
- [frame-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object.cc)
- [module-manager.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.h)
- [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)
- [exception-state.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-state.h)
- [exception-state.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-state.cc)
- [string-table.h](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.h)
- [string-table.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/string-table.cc)
- [builtins](file:///e:/MyProject/S.A.A.U.S.O/src/builtins)
- [execution](file:///e:/MyProject/S.A.A.U.S.O/src/execution)
- 以及所有持有 `Tagged<T>` 的 C++ 标准库容器使用点

### 7.3 为什么改或审计这些文件

- `Heap::IterateRoots()` 是现有 root tracing 总入口。
- `Isolate` 持有多个长期对象，但目前 `Isolate::Iterate()` 的覆盖范围需要再次确认。
- `HandleScopeImplementer` 与 `GlobalHandles` 是 native 层对象保活主链。
- `Interpreter` 与 `FrameObject` 是执行栈保活关键路径。
- `ModuleManager` 影响 `sys.modules`、内建模块与 import 缓存。
- `StringTable` 目前依赖“都在 MetaSpace”的假设，是典型的未来风险点。
- `builtins` 初始化链路可能存在 bootstrap 阶段缓存或静态持有点，必须确认不会绕开主 root tracing。
- C++ 标准库容器中的 `Tagged<T>` 若被 runtime 长期持有，也必须被明确纳入 roots 模型。
- `TryCatch` 与异常展开中的瞬时对象引用，必须确认是否已经被 `HandleScope` 或 `ExceptionState` 稳定覆盖。

### 7.4 主要产出

- 一份“当前 roots 覆盖矩阵”。
- 一份“依赖 MetaSpace 假设的 root 项”清单。
- 一份“未来 major GC 需要新增或强化的 Iterate 入口”清单。
- 一份“C++ 标准库容器持有 `Tagged<T>`”清单。
- 一份“bootstrap / 静态缓存 / TryCatch 瞬时引用”清单。

### 7.5 风险点

- 如果 Phase 0 不把 roots 清单做完整，后续 mark-sweep 调试会极难定位。
- 如果未来对象从 MetaSpace 挪到普通堆，而 StringTable root tracing 仍未开放，会形成隐蔽回收错误。
- 如果遗漏标准库容器、bootstrap 缓存或瞬时异常引用，major GC 未来会出现“看似 roots 完整但仍偶发悬挂”的高成本问题。

## 8. W3：地址语义与未来 Identifier 审计

### 8.1 目标

- 找出所有依赖对象物理地址的语义点。
- 区分“仅调试可见性用途”和“真实语言语义绑定”。
- 为未来引入懒加载 Identifier 设计预留清单。

### 8.2 预计重点审计文件

- [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc)
- [runtime-py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-function.cc)
- [py-type-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc)
- [py-oddballs-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-oddballs-klass.cc)
- [builtins-definitions.h](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-definitions.h)
- [klass-vtable.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.h)

### 8.3 为什么改或审计这些文件

- 默认 `object repr`、`function repr`、`method repr` 目前直接打印地址。
- `type.__hash__` 与 `None.__hash__` 直接基于地址。
- `id()` 当前尚未实现，这反而给了项目在 Phase 0 先冻结正确语义的机会。
- `klass-vtable` 当前没有 `__del__` 槽位，说明“不支持 finalizer”可以较低成本地正式写进架构约束。

### 8.4 主要产出

- 一份“地址参与语义的代码点清单”。
- 一份“未来必须替换为 identifier 或 side table 的候选点清单”。
- 一份“`__del__` 不支持策略”的文档落点清单。

### 8.5 风险点

- 如果不区分 `repr` 的调试输出和 `hash/id` 的语言语义，后续 moving GC 讨论会长期混乱。
- 如果未来先实现 `id()`，再去改语义，会平白引入兼容债。

## 9. W4：测试护栏补齐

### 9.1 目标

- 为 Phase 1 以后可能引入的结构性改动建立回归护栏。
- 尽早让“字段私有化、setter 收口、屏障模板化”具备编译和行为验证。

### 9.2 预计涉及文件

- [gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc)
- [global-handle-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/handles/global-handle-unittest.cc)
- [interpreter-custom-class-dunder-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-custom-class-dunder-unittest.cc)
- 以及后续可能新增的：
- `test/unittests/heap/remembered-set-unittest.cc`
- `test/unittests/objects/object-setter-unittest.cc`
- `test/unittests/heap/root-coverage-unittest.cc`

### 9.3 为什么改这些文件

- 现有 `gc-unittest` 和 `global-handle-unittest` 适合补“不要破坏当前 minor GC 行为”的回归护栏。
- 需要新增更细粒度测试，覆盖 setter-only 写入、remembered set 清理、root coverage 假设。

### 9.4 风险点

- 如果不先补护栏，Phase 0 的结构性重构容易在“编译通过但行为悄悄退化”的情况下进入后续阶段。
- 如果测试只覆盖 minor GC，而不覆盖“字段访问路径变化”，就很难及时发现 setter 重构遗漏。

## 10. 建议的文件优先级

### P0：优先立即勘探与改造

- [heap.h](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.h)
- [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc)
- [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc)
- [py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h)
- [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc)
- [py-dict.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.h)
- [py-dict.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict.cc)
- [py-function.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.h)
- [py-function.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-function.cc)
- [klass.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.h)
- [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)
- [py-type-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.h)
- [py-type-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object.cc)
- 所有持有 `Tagged<T>` 的标准库容器使用点
- builtins/bootstrap 缓存持有点
- TryCatch / 异常展开瞬时引用持有点

### P1：在第一批规范稳定后推进

- [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h)
- [py-code-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.cc)
- [method-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/method-object.h)
- [method-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/method-object.cc)
- [py-dict-views.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.h)
- [py-dict-views.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-dict-views.cc)
- [py-list-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.h)
- [py-list-iterator.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator.cc)
- [py-tuple-iterator.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.h)
- [py-tuple-iterator.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-iterator.cc)

### P2：文档和测试同步收尾

- [GC-and-Heap.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/GC-and-Heap.md)
- [Object-System.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Object-System.md)
- [Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)
- [gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc)
- [global-handle-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/handles/global-handle-unittest.cc)

## 11. 建议的执行顺序

### Step 1：先改规范，再改代码

- 先更新文档与工程约束。
- 明确 side metadata / bitmap 是默认方向。
- 明确 `__del__` 不支持。
- 明确 future `id()` 与对象物理地址解耦。

### Step 2：建立 setter 模板

- 以 `FixedArray`、`Cell`、`PyObject::SetProperties`、`PyTuple::Set` 为模板整理统一写法。

### Step 3：治理第一批高风险对象

- `list`
- `dict`
- `function`
- `klass`
- `type-object`
- 并同步修改 `factory.cc` 中对应对象的初始化与发布路径

### Step 4：做 roots 与地址语义清单

- 输出审计结果，而不是只停留在口头共识。

### Step 5：补测试护栏

- 在进入 Phase 1 前，保证结构性改造有回归覆盖。

## 12. Phase 0 的完成标准

- 所有关键架构决议都已固化到正式文档。
- setter 与字段封装规范已经建立，并至少在第一批核心对象上落地。
- 未来 write barrier 接入点已经具备清晰结构。
- roots 审计与地址语义审计已经形成书面清单。
- 标准库容器、bootstrap 缓存与 TryCatch 瞬时引用均已进入 roots 清单。
- 对 Phase 1 的输入条件已经明确，而不是模糊依赖“后面再看”。

## 13. 一句话结论

Phase 0 的本质不是“开始写 GC”，而是“先把会让 GC 失败的工程因素系统性清理掉”。  
如果这一阶段做得扎实，后续 `OldSpace`、promotion 和 `mark-sweep` 的实现难度会明显下降；如果这一阶段做得草率，后面每一阶段都会反复返工。
