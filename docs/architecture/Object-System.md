# 对象系统（Object System）

本文档介绍 S.A.A.U.S.O 的对象系统设计思路，并提供实现索引，面向新增/维护内建对象与 klass/vtable/slot 的贡献者。

对象系统的“必须遵守”的工程约束（例如 vtable slot 契约、cast like/exact、对象布局与 Tagged/Smi 编码）请以[Saauso-VM-Engineering-Constraints.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-Engineering-Constraints.md)为准。

## 1. 基本概念与分工

- **PyObject**：所有堆对象的基类。
- **Klass**：表示对象的类型/元信息（类似于 V8 的 Map 或 HotSpot 的 Klass）。数据（`PyObject`）与行为（`Klass`）严格分离。
- **Object**：仅作为 C++ 侧的轻量基类（见 `src/objects/objects.h`），不要与 Python 堆对象基类 `PyObject` 混淆。
- **VirtualTable（slot 表）**：
  - `Klass::vtable_` 保存各类操作的函数指针，优先使用以 `Handle<PyObject>` 为参数的 SAFE 版本，避免 GC 导致悬垂引用。
  - vtable 的 slot 设计是一组“可失败 API 契约”，很多 slot 会返回 `Maybe<bool>` / `MaybeHandle<PyObject>`；调用方必须区分“正常 false”和“异常失败”，避免把 pending exception 误判为普通 miss。

## 2. 内建类型构造函数（`new_instance/init_instance`）的原理与写法

本项目中，“调用一个 type object”（例如 `list(...)` / `dict(...)` / 自定义类 `C(...)`）会统一走两阶段构造：`new_instance`（分配/构造对象） + `init_instance`（初始化与 `__init__` 调用）。

### 2.1 调用链（从 Python 层到 C++ 构造 slot）

- 字节码层：`CALL` 触发一次可调用对象调用；当被调用对象是 type object 时，进入 type object 的 call 语义，并进一步触发 `new_instance/init_instance`。
- 若被调用对象是 type object：走 [PyTypeObjectKlass::Virtual_Call](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc#L176-L181)。

调用协议（尤其是 CPython3.12 的 call-shape 双槽位协议）属于解释器实现细节，见：

- [Interpreter.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Interpreter.md)

### 2.2 默认实现的语义（何时会被用到）

- `new_instance/init_instance` 的默认语义来源于 `object` 的实现：见 [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L257-L313) 中的 `Generic_NewInstance/Generic_InitInstance`。
- 动态创建 Python 类时，`Runtime_CreatePythonClass` 会在 `OrderSupers()` 之后调用 `klass->InitializeVtable()`，由 [klass-vtable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc#L17-L89) 按 MRO 继承 slot 并依据“当前类字典中的 magic method”覆盖为 trampoline。
- 因此，普通 Python 类在未显式重写 `__new__/__init__` 时会沿继承链落到 `object` 默认语义；需要特定内存布局的内建容器/数值类型仍应在各自 `Klass` 中覆盖。

### 2.3 为什么内建类型必须覆盖

- 例如 `list/tuple/dict` 这些类型有自己的字段布局（如 `PyList::array_`、`PyDict::data_`），必须覆盖 `new_instance` 以分配正确布局对象；并按语义覆盖 `init_instance` 处理参数与 `__init__` 分发。

### 2.4 slot 覆盖时机（初始化顺序很关键）

- 对内建 `Klass`：`Isolate::InitMetaArea()` 在预初始化阶段会调用各类 `PreInitialize()`，由其直接填充 vtable（见 [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc#L241-L267) 与 [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L40-L55)）。
- 对运行时动态创建的 Python 类：覆盖关系在 `Runtime_CreatePythonClass -> OrderSupers -> InitializeVtable` 这条链路中完成，而不是走 `PreInitialize()`。
- 若内建类型忘记覆盖 `new_instance/init_instance`，语义会沿继承链回退到基类实现（通常是 `object` 的默认实现）。

### 2.5 推荐实现模板（强调可测性）

- 在 `*-klass.h` 中声明：
  - `static MaybeHandle<PyObject> Virtual_NewInstance(Isolate* isolate, Tagged<Klass> klass_self, Handle<PyObject> args, Handle<PyObject> kwargs);`
  - `static MaybeHandle<PyObject> Virtual_InitInstance(Isolate* isolate, Handle<PyObject> instance, Handle<PyObject> args, Handle<PyObject> kwargs);`
- 在 `PreInitialize()` 中接入：
  - `vtable_.new_instance_ = &Virtual_NewInstance;`
  - `vtable_.init_instance_ = &Virtual_InitInstance;`
- 在实现中处理参数的通用约定：
  - `args` 通常是 `PyTuple` 或 null；`kwargs` 通常是 `PyDict` 或 null。
  - `argc = args.is_null() ? 0 : args->length()`；对“最多 1 个位置参数”等规则做 fail-fast 校验。
  - 对“不支持关键字参数”的构造函数：当 `kwargs` 非空且 `occupied()!=0` 时，抛 `TypeError` 并返回空 `MaybeHandle`（保持 pending exception 传播）。
  - 尽量复用 runtime helper：例如 iterable 展开使用 [Runtime_UnpackIterableObjectToTuple](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-iterable.cc#L43-L64)，list 扩展使用 `Runtime_ExtendListByItratableObject`，避免在类型实现中重复写迭代逻辑。

### 2.6 语义对齐要点（建议以解释器端到端用例覆盖）

- `tuple(tuple_obj)` 返回同一对象（不可变类型的语义/优化）。
- `list(list_obj)` 返回新对象（浅拷贝元素引用）。
- `dict(old_dict)` 返回新对象（浅拷贝键值对引用），随后 `kwargs` 覆盖之前的值。
- 对齐的关键不是“支持更多输入”，而是“正确的参数个数规则 + kwargs 规则 + fast path/identity 语义 + 错误文案”。

### 2.7 测试策略（强烈建议）

- 构造函数属于“解释器 builtins + type object call + klass new/init slot”整条链路的交汇点，优先写解释器端到端测试（`RunScript(...)` + `ExpectPrintResult(...)`），以便一次性覆盖：
  - `builtins` 注入是否完整（例如 `tuple/list/dict` 是否在 builtins 中可见）。
  - `CALL` 参数布局是否正确。
  - `new_instance/init_instance` 的语义与报错是否符合预期。
- `Klass::Iterate(ObjectVisitor*)` 是 GC 正确性的关键路径：新增字段时必须同步更新。

