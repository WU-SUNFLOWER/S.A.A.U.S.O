# S.A.A.U.S.O 基础容器 GC 风险清单与优先级治理路线图

本文档基于对当前仓库中四类基础容器 `PyList`、`PyTuple`、`FixedArray`、`PyString` 的静态审查结果，整理它们在当前 VM 堆模型下的 GC 安全风险、风险分级、证据与后续治理优先级。

审查口径与前提：

- 当前仓库主路径仍以 **new-space minor scavenge** 为主，对象可能在 GC 中搬迁。
- `Handle<T>` 是跨 GC 安全持有方式；`Tagged<T>` 等价于“带语义的裸指针”，不适合跨可分配调用长期保存。
- 重点审查以下风险模式：
  - 成员函数内部隐式 `this` 在可分配调用后继续被使用。
  - 裸 `Tagged` / 裸 buffer / 裸尾随元素指针跨可分配调用继续被使用。
  - API 看似“只读/轻量”，实际上会触发分配、异常传播或 Python 语义调用。
  - 当前实现虽然未立即出错，但接口过于锋利，后续迭代极易回退。

## 1. 总览结论

### 1.1 风险优先级

| 优先级 | 容器 | 当前结论 | 是否建议优先治理 |
| --- | --- | --- | --- |
| P0 | `PyString` | 存在多处真实 GC 不安全模式 | 是 |
| P1 | `PyTuple` | 主路径基本安全，但 tuple-like 子类构造有明确构造期风险 | 是 |
| P2 | `PyList` | 主路径基本安全，但存在高风险逻辑缺口与若干脆弱 API | 是 |
| P3 | `FixedArray` | 当前未发现明确 GC 缺陷，但属于高风险底层 raw 容器 | 暂缓 |

### 1.2 简要判断

- `PyString`：风险最高。多处把 `buffer()` 返回的裸字符指针传给会再次分配字符串的接口，属于真实 GC 不安全点。
- `PyTuple`：普通 `tuple-exact` 路径基本安全；但 `Factory::NewPyTupleLike()` 在对象尚未进入 handle roots 前可能执行额外分配，存在明确的构造期对象漂移风险。
- `PyList`：大多数可分配路径已经使用 `Handle<PyList>` / `Handle<PyObject>` 驱动，没有发现 PyDict 旧版那类系统性 this 漂移问题；但 `extend(self)` 逻辑存在严重缺口，`PopTagged/GetTagged` 风格也偏脆弱。
- `FixedArray`：当前实现整体基本可靠，但它是其它容器的底层承载体，任何不当使用都会把 GC 风险外溢到上层。

## 2. 分容器风险清单

## 2.1 PyString

### 2.1.1 风险等级

- **总体等级：高**
- **治理优先级：P0**

### 2.1.2 当前主要风险

#### 风险 A：裸 `buffer()` 跨字符串新分配

这是当前最严重、也最值得优先处理的问题。

典型模式：

- 从现有 `PyString` 取得 `const char*` 源缓冲；
- 把该裸指针传给会再次分配新 `PyString` 的接口；
- 如果新分配过程中触发 GC，而源字符串位于新生代，则源对象可能搬迁，旧 `const char*` 立即悬空。

已命中的典型位置：

- `PyString::Clone()`： [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L62-L67)
- `PyStringKlass::Virtual_Subscr()`： [py-string-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string-klass.cc#L305-L319)
- `str.upper()`： [builtins-py-string-methods.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-py-string-methods.cc#L67-L90)
- 这些路径最终都会走 `Factory::NewString(...)`： [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc#L307-L314)

#### 风险 B：对象方法广泛依赖成员 `this` 稳定

当前 `PyString` 仍保留大量成员方法：

- `GetHash()`： [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L136-L144)
- `IsEqualTo()`： [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L150-L166)
- `IndexOf/LastIndexOf/Contains()`： [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L227-L265)

这些函数大多数当前“不分配”，所以短期内通常不会因 `this` 漂移而炸掉；但它们的 API 形状对后续维护不友好。只要未来有人在函数体中插入分配或异常路径，就会把潜在问题转化为真实问题。

#### 风险 C：`buffer()` / `writable_buffer()` 暴露过于锋利

接口位置： [py-string.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.h#L74-L78)

当前字符串实现是“尾随 buffer 内联对象”的顺序布局，所以 `buffer()` 能工作；但这类接口：

- 强依赖当前对象表示；
- 容易让调用方形成“拿到 buffer 就可长期安全使用”的错误心智模型；
- 将来若引入 sliced/cons/flattened string，扩展难度会很大。

### 2.1.3 当前相对安全的路径

- `Slice()`：先分配结果，再复制源内容，见 [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L206-L219)
- `Append()`：最终走 `NewConsString()`，先分配结果，再复制左右串，见 [py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-string.cc#L221-L225)
- `repr()`：先在宿主侧 `std::string` 组装，再一次性创建 `PyString`，见 [runtime-py-string.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-py-string.cc#L32-L58)

### 2.1.4 治理建议

- **必须做**
  - 禁止“`buffer()` 作为源指针跨新字符串分配”模式。
  - 逐个修复 `Clone`、`str[i]`、`upper()` 等路径。
- **建议做**
  - 逐步减少成员风格的 GC-sensitive API。
  - 明确区分“仅局部短期读 buffer”与“跨分配持有 buffer”的禁用边界。
- **可选做**
  - 以长期演进为目标，开始筹备 `TryGetRawBuffer()/FlattenAndGetBuffer()` 风格的接口收口。

## 2.2 PyTuple

### 2.2.1 风险等级

- **总体等级：中高**
- **治理优先级：P1**

### 2.2.2 当前主要风险

#### 风险 A：tuple-like 子类构造期间对象尚未 rooted 即再次分配

问题位置： [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc#L233-L257)

当前 `NewPyTupleLike()` 的逻辑是：

- 先 `AllocateRaw(...)` 得到裸 `Tagged<PyTuple> object`
- 在禁分配区初始化 `length_`、klass、properties 与尾随元素
- 若 `klass_self->instance_has_properties_dict()` 为真，则调用 `NewPyDict(...)`
- 最后才 `handle(object, isolate_)`

风险点在于：

- 在 `object` 还没有进入 handle roots 之前；
- 已经执行了额外分配；
- 若此处触发 GC，`object` 可能搬迁，从而使局部裸对象地址失效。

该风险对普通 `tuple-exact` 不一定触发，但对 tuple 子类路径是真实可达的。对应实例化入口可见： [py-tuple-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-klass.cc#L131-L185)

### 2.2.3 当前相对安全的路径

- 普通 tuple 构造与 `SetInternal` 初始化本身基本安全。
- 元素扫描遍历正确，见 [py-tuple-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-tuple-klass.cc#L293-L301)
- 常规 `Get/GetTagged/迭代/拷贝/list->tuple` 主路径未见明显“裸元素跨可分配调用”的问题。

### 2.2.4 治理建议

- **必须做**
  - 修正 `NewPyTupleLike()` 的构造顺序：对象必须先 root 化，再做可能分配的 `__dict__` 初始化。
- **建议做**
  - 补一个 tuple 子类构造 + `sysgc()` / 分配压力回归用例，专门覆盖这个窗口。
- **可选做**
  - 后续再评估是否需要统一压缩 `GetTagged` 的使用面。

## 2.3 PyList

### 2.3.1 风险等级

- **总体等级：中**
- **治理优先级：P2**

### 2.3.2 当前主要风险

#### 风险 A：`extend(self)` fast path 存在严重逻辑缺口

问题位置： [runtime-iterable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-iterable.cc#L33-L39)

当前 `Runtime_ExtendListByItratableObject()` 的 list fast path：

- 若 `iteratable` 是 list，则 `source = Handle<PyList>::cast(iteratable)`
- 循环条件为 `i < source->length()`
- 每轮调用 `PyList::Append(list, source->Get(i, isolate), isolate)`

若 `source` 与 `list` 是同一个对象，则：

- `Append` 会持续增长 `source->length()`
- 循环上界也持续增长
- 导致非终止 / 失控扩容 / 极端 GC 压力

这不是典型的 UAF，但它是基础容器级的严重稳定性问题。

#### 风险 B：`PopTagged()` / `GetTagged()` 风格脆弱

接口位置：

- [py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h#L29-L36)
- [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L35-L60)

当前 `Pop()` 是：

- 先 `PopTagged()`
- 再 `handle(...)`

因为中间当前没有可分配调用，所以“现在没事”；但这种模式与 PyDict 已完成的收口方向并不一致，未来极易回退。

#### 风险 C：删除后不清尾槽，导致额外对象保活

位置：

- `PopTagged()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L39-L42)
- `RemoveByIndex()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L84-L92)
- `Clear()`： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L105-L107)

这不会破坏 GC 正确性，但会让原本已经“逻辑删除”的元素继续被保活。

### 2.3.3 当前相对安全的路径

- 构造： [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc#L201-L225)
- 扩容： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L169-L178)
- 查找： [py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L118-L131)
- 排序主链：比较前会把元素 handle 化，长度变化也有防御性检查

### 2.3.4 治理建议

- **必须做**
  - 修掉 `extend(self)` fast path。
- **建议做**
  - 评估是否逐步淘汰 `PopTagged()` / `GetTagged()` 作为公开主接口。
  - 评估 `Clear/Pop/RemoveByIndex` 是否需要清尾槽。
- **可选做**
  - 后续参照 PyDict 的治理经验，把 list 的 GC-sensitive 原语再做一次 API 收口。

## 2.4 FixedArray

### 2.4.1 风险等级

- **总体等级：较低**
- **治理优先级：P3**

### 2.4.2 当前主要风险

`FixedArray` 当前没有发现明确的现存 GC 缺陷，问题更多在于它是一个非常锋利的底层 raw 容器：

- `Get/Set` 只做边界断言，不做更强的语义保护： [fixed-array.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.cc#L33-L46)
- `data()` 直接暴露尾随元素首地址： [fixed-array.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array.h#L29-L33)
- 上层若错误地拿到裸元素后跨可分配调用继续使用，风险会直接外溢到容器外层。

### 2.4.3 当前相对安全的路径

- 构造： [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc#L124-L145)
- 扩容复制： [factory.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/factory.cc#L147-L167)
- 遍历： [fixed-array-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/fixed-array-klass.cc#L57-L66)

### 2.4.4 治理建议

- **必须做**
  - 当前无。
- **建议做**
  - 把它视为“高杠杆底层基建”，补充更明确的使用约束文档。
- **可选做**
  - 未来若推进全仓 raw/handle discipline 统一治理，再一并收口。

## 3. 优先级治理路线图

## 3.1 Phase 0：统一评审口径

目标：先把容器治理的统一标准写清楚，避免一个容器修完、另一个容器继续新增同类问题。

建议规则：

- 所有 **GC-sensitive** 容器原语，优先使用 `static + Handle<Container>` 形态。
- 禁止把 `Tagged<PyObject>`、裸 `buffer()`、尾随元素首地址等跨可分配调用长期保存。
- 如果某个 API 语义上可能触发 Python 代码、分配或异常传播，就不应设计成会诱导调用方持有裸对象的“轻量接口”。

## 3.2 Phase 1：优先治理 PyString（P0）

目标：先清掉已知真实危险模式。

建议顺序：

1. 修 `PyString::Clone()`。
2. 修 `PyStringKlass::Virtual_Subscr()` 的单字符返回路径。
3. 修 `str.upper()`。
4. 全仓检索“`buffer()` 作为新字符串源指针”模式并批量消除。

建议完成标准：

- 不再出现“从现有 `PyString` 取裸 `buffer()` 后，再调用 `NewString/New(...)`”的模式。
- 补针对性回归测试：
  - `str[i]` 在 GC 压力下仍稳定；
  - `upper()` 在 GC 压力下仍稳定；
  - `Clone()` 相关路径在 GC 压力下仍稳定。

## 3.3 Phase 2：修 PyTuple 子类构造（P1）

目标：消除明确的构造期未 root 分配窗口。

建议动作：

- 重写 `NewPyTupleLike()` 的构造顺序，对象必须先 handle 化，再做 `instance_has_properties_dict()` 分支中的额外分配。

建议完成标准：

- tuple 子类构造路径中，不再存在“裸 `Tagged<PyTuple>` 跨分配”的窗口。
- 补一个 tuple 子类 + `sysgc()` / 垃圾分配压力测试。

## 3.4 Phase 3：治理 PyList（P2）

目标：先处理真正危险的逻辑缺口，再处理脆弱 API。

建议顺序：

1. 修 `extend(self)` fast path。
2. 评估 `PopTagged()` / `GetTagged()` 是否继续保留。
3. 评估是否在 `Clear/Pop/RemoveByIndex` 中清尾槽。

建议完成标准：

- `extend(self)` 行为可预期，不会无限增长。
- list 的高频读取/弹出接口不再鼓励调用方持有裸 `Tagged`。

## 3.5 Phase 4：整理 FixedArray 约束（P3）

目标：不是立刻大改 FixedArray，而是给未来维护设边界。

建议动作：

- 在工程文档中明确：
  - `FixedArray` 是 raw 容器，不代表“GC-safe 持有”
  - 上层若跨可分配调用持有其元素，必须先 handle 化
- 在新 PR / 新实现评审中，把 `FixedArray` 看作“危险底层工具”。

## 4. 推荐新增/补强的回归测试

## 4.1 PyString

- `str[i]` 在 `sysgc()` 压力下稳定
- `str.upper()` 在 `sysgc()` 压力下稳定
- `Clone` / `repr` / 异常格式化路径涉及 `Clone` 时稳定

## 4.2 PyTuple

- tuple 子类构造过程中触发 GC 后仍可正确访问元素与实例属性

## 4.3 PyList

- `list.extend(self)` 行为测试
- `pop/get/remove/index` 与自定义 `__eq__` / `__iter__` / `sysgc()` 交互测试
- `clear/remove/pop` 后对象保活行为测试（若后续决定清尾槽）

## 4.4 FixedArray

- 当前可暂不单独补对象层 GC 测试；优先依赖上层容器对象图回归

## 5. 发起治理时的建议顺序

如果后续要像治理 PyDict 一样系统推进，我建议按以下顺序落地：

1. **PyString**
2. **PyTuple 子类构造**
3. **PyList**
4. **FixedArray 使用规范**

原因：

- `PyString` 当前最接近“已经存在真实 GC 不安全模式”的状态。
- `PyTuple` 的问题点集中、修复收益高。
- `PyList` 目前更多是逻辑缺口与接口脆弱性，不如前两者紧急。
- `FixedArray` 更适合作为最后的基建层文档与审查约束收尾项。

## 6. 最终建议

- 当前不建议把四类容器打包成一个超大 MR 同时治理。
- 最稳妥的做法是：
  - 先把 `PyString` 作为下一批主线治理对象；
  - 然后单独修 `PyTuple` 构造期风险；
  - 再回头整理 `PyList` 的逻辑与 API 形状；
  - 最后以文档和评审规则形式收住 `FixedArray`。

- 如果目标是“防止 VM 从底层腐烂”，那么应该把下面这条当作仓库级原则：

> **任何基础容器中，只要某条路径可能触发 Python 代码、分配或异常传播，就不得依赖裸 `this`、裸 `Tagged`、裸 `buffer` 或裸尾随元素指针跨该路径存活。**
