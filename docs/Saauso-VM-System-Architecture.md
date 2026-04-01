# S.A.A.U.S.O VM 系统架构

本文档为 S.A.A.U.S.O 的系统架构指南，用于介绍解释模块分层、依赖边界、错误传播约定、关键执行协议与核心子系统（GC、解释器、模块系统、前端等）的职责分配，并提供实现索引。

## 1. 分层与依赖方向（V8-like）

本项目的架构设计深度借鉴了 V8 和 HotSpot。

- **目标**：让“对象表示/运行时语义/执行引擎/系统装配”各司其职，避免 `src/objects` 因承载高层语义而膨胀，并把变化点（语义/报错/协议）收敛在更合适的层。
- **utils (`src/utils/`)**：纯工具层；严禁依赖 VM 上层能力（对象/堆/句柄/解释器/运行时/模块系统等）。
- **handles (`src/handles/`)**：GC 安全持有与跨 `HandleScope` 约束；可依赖 `execution` 的 `Isolate::Current()` 绑定模型（现有实现如此）。
- **objects (`src/objects/`)**：对象表示层（布局/字段/GC iterate/instance_size）+ vtable slot 的调度骨架；`py-xxx.cc` 实体类优先保持为“数据容器 + 原语操作”，避免把跨对象的复杂语义流程塞进实体类。
- **execution (`src/execution/`)**：运行时容器与门面层；持有 Isolate/ExceptionState，并通过 `Execution`（AllStatic）收敛“执行/调用”入口。
- **builtins (`src/builtins/`)**：Python API 外壳层；负责参数解析与错误文案；尽量薄，复杂流程优先调用 runtime helper。
- **runtime (`src/runtime/`)**：跨类型语义层；承载可复用流程（iterable unpack、`str(x)`、magic method invoke、建类/反射等）。
- **interpreter (`src/interpreter/`)**：执行层；依赖 objects/runtime/builtins；负责字节码调度、调用协议与栈帧，并消费 Isolate::ExceptionState 做异常展开。

### 1.1. Architecture Boundaries（必须遵守）

- **禁止**：`src/runtime` 与 `src/builtins` 直接 include 或调用 `src/interpreter`（包括 `#include "src/interpreter/interpreter.h"`、`Isolate::Current()->interpreter()->...` 等）。
- **允许**：`src/runtime` 与 `src/builtins` 通过 execution/runtime 提供的门面 API 完成调用与抛错：
  - 执行/调用入口：使用 `src/execution/execution.h` 的 `Execution::*`
  - 抛错与错误格式化：使用 `src/runtime/runtime-exceptions.h` 的 `Runtime_Throw*` / `Runtime_FormatPendingExceptionForStderr`
  - error indicator：由 `Isolate::exception_state()` 统一持有与传播
- **理由**：
  - 让依赖方向保持单向：`runtime/builtins -> execution -> interpreter`，避免循环依赖与“runtime 变半个解释器”。
  - 降低后续演进成本：未来引入其他执行器（AOT/JIT）时，只需要调整 execution 门面的实现，而不是全仓库逐点替换调用。
  - 控制职责边界：Isolate 负责状态（如 ExceptionState 与 builtins），Execution 负责门面（Call/CurrentFrame*），interpreter 负责算法（dispatch/unwind）。

### 1.2. Error Conventions（必须遵守）

- **Fallible API（可能抛 Python 异常）**：
  - 返回对象句柄时使用 `MaybeHandle<T>`；返回状态值时使用 `Maybe<T>`/`Maybe<void>`
  - 失败时必须返回空值（`kNullMaybeHandle` 或 `kNullMaybe`），并且必须已设置 `Isolate::exception_state()` 的 pending exception
  - 调用方必须显式检查失败态（`IsEmpty()/ToHandle(...)` 或 `IsNothing()`）并继续向上返回失败或进入 unwind
- **Try/Lookup API（miss 不是异常，但过程仍可能抛异常）**：
  - miss 必须可继续执行（不得设置 pending exception）
  - 若查询过程可能触发 Python 代码/异常（例如 `__getattr__`、`__hash__/__eq__` 等），必须使用 `Maybe<T>`（三态）消除“未命中 vs 异常”的二义性
  - 推荐模式：返回 `Maybe<bool>` + out 参数
    - `Nothing`：查询过程中发生异常（已设置 pending exception，out 为 null）
    - `true`：命中（out 非 null）
    - `false`：未命中（out 为 null，且不得设置 pending exception）
- **执行入口**：
  - 调用 Python（CallPython/执行 code）统一走 `Execution::*` 与 `MaybeHandle`
  - 需要抛出 Python 异常时统一使用 `Runtime_Throw*`（`src/runtime/runtime-exceptions.h`）
  - 建议优先使用 `ASSIGN_RETURN_ON_EXCEPTION*` 宏处理“调用失败即向上返回”的样板代码，该宏约定返回 `kNullMaybe`（可被隐式转换为“空 MaybeHandle”）

### 1.3. dict API Conventions（必须遵守）

- **优先使用 runtime 语义入口**（当你需要 Python 语言层语义时）：
  - `Runtime_DictGetItem/Runtime_DictSetItem/Runtime_DictDelItem`：对应 `d[key]`/写入/删除的 KeyError 语义，并保证 `__hash__/__eq__` 异常原样传播。
- **允许直接使用 PyDict 原语的场景**（当你实现的是解释器更高层语义时）：
  - 例如 `LOAD_NAME/LOAD_GLOBAL` 的 locals→globals→builtins 链式查找（miss 继续，最终 `NameError`），该语义不等价于 `d[key]`。
  - 这类场景必须使用 fallible API：`Get/GetTagged/Put/Remove/ContainsKey`，并在失败时立即传播 pending exception。
- **内部初始化路径的约束**：
  - 即便是 key 可证明为 interned `PyString`（例如 `ST(...)` / 字节码 names 表），也必须使用 `Put/Get/GetTagged/...`。
  - 若调用点没有异常传播通道，可显式忽略返回值，但不得恢复旧 API。

## 2. 对象系统 (`src/objects`)

- **PyObject**: 所有堆对象的基类。
- **Klass**: 表示对象的类型/元信息（类似于 V8 的 Map 或 HotSpot 的 Klass）。数据（`PyObject`）与行为（`Klass`）严格分离。
- **Object**: 仅作为 C++ 侧的轻量基类（见 `src/objects/objects.h`），不要与 Python 堆对象基类 `PyObject` 混淆。
- **VirtualTable (slot 表)**:
  - `Klass::vtable_` 保存各类操作的函数指针，优先使用以 `Handle<PyObject>` 为参数的 SAFE 版本，避免 GC 导致悬垂引用。
  - 新增/重写 slot 时：要么显式指向默认实现，要么确保所有调用点在 `nullptr` 时可处理（否则会空函数指针调用崩溃）。
  - `instance_size` 必须为“不可触发 GC”的纯计算；`iterate` 必须遍历对象内部所有 `Tagged<PyObject>` 引用字段。
  - `getattr` 采用四参形式：`getattr(self, name, is_try, out)`，返回 `Maybe<bool>`（三态）：
    - `Nothing`：查询过程中发生异常（已设置 pending exception）
    - `true`：命中属性，`out` 写入属性值（非 null）
    - `false`：未命中属性，`out` 为 null；若 `is_try=false`，实现必须设置 `AttributeError` 并返回 `Nothing`
  - 比较、`contains` 与算数/下标等可能抛异常的热点 slot 已统一使用 `Maybe<bool>` / `MaybeHandle<PyObject>`；调用方必须区分“正常 false”和“异常失败”，避免把 pending exception 误判为普通 miss。

### 2.1. 内建类型构造函数（`new_instance/init_instance`）的原理与写法

本项目中，“调用一个 type object”（例如 `list(...)` / `dict(...)` / 自定义类 `C(...)`）会统一走两阶段构造：`new_instance`（分配/构造对象） + `init_instance`（初始化与 `__init__` 调用）。

**调用链（从 Python 层到 C++ 构造 slot）**

- 字节码层：`CALL` 触发一次可调用对象调用（见 3.4.1 的双槽位协议）。
- 若被调用对象是 type object：走 [PyTypeObjectKlass::Virtual_Call](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-type-object-klass.cc#L176-L181)。

**默认实现的语义（何时会被用到）**

- `new_instance/init_instance` 的默认语义来源于 `object` 的实现：见 [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L257-L313) 中的 `Generic_NewInstance/Generic_InitInstance`。
- 动态创建 Python 类时，`Runtime_CreatePythonClass` 会在 `OrderSupers()` 之后调用 `klass->InitializeVtable()`，由 [klass-vtable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass-vtable.cc#L17-L89) 按 MRO 继承 slot 并依据“当前类字典中的 magic method”覆盖为 trampoline。
- 因此，普通 Python 类在未显式重写 `__new__/__init__` 时会沿继承链落到 `object` 默认语义；需要特定内存布局的内建容器/数值类型仍应在各自 `Klass` 中覆盖。

**为什么内建类型必须覆盖**

- 例如 `list/tuple/dict` 这些类型有自己的字段布局（如 `PyList::array_`、`PyDict::data_`），必须覆盖 `new_instance` 以分配正确布局对象；并按语义覆盖 `init_instance` 处理参数与 `__init__` 分发。

**slot 覆盖时机（初始化顺序很关键）**

- 对内建 `Klass`：`Isolate::InitMetaArea()` 在预初始化阶段会调用各类 `PreInitialize()`，由其直接填充 vtable（见 [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc#L241-L267) 与 [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L40-L55)）。
- 对运行时动态创建的 Python 类：覆盖关系在 `Runtime_CreatePythonClass -> OrderSupers -> InitializeVtable` 这条链路中完成，而不是走 `PreInitialize()`。
- 若内建类型忘记覆盖 `new_instance/init_instance`，语义会沿继承链回退到基类实现（通常是 `object` 的默认实现）。

**推荐实现模板（遵循仓库习惯，强调可测性）**

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

**对齐 CPython 的典型语义要点（实现时要考虑）**

- `tuple(tuple_obj)` 返回同一对象（不可变类型的语义/优化）。
- `list(list_obj)` 返回新对象（浅拷贝元素引用）。
- `dict(old_dict)` 返回新对象（浅拷贝键值对引用），随后 `kwargs` 覆盖之前的值。
- 对齐的关键不是“支持更多输入”，而是“正确的参数个数规则 + kwargs 规则 + fast path/identity 语义 + 错误文案”，这些应当通过解释器端到端单测覆盖。

**测试策略（强烈建议）**

- 构造函数属于“解释器 builtins + type object call + klass new/init slot”整条链路的交汇点，优先写解释器端到端测试（`RunScript(...)` + `ExpectPrintResult(...)`），以便一次性覆盖：
  - `builtins` 注入是否完整（例如 `tuple/list/dict` 是否在 builtins 中可见）。
  - `CALL` 参数布局是否正确。
  - `new_instance/init_instance` 的语义与报错是否符合预期。
- **Klass 也是 GC Root**:
  - `Klass::Iterate(ObjectVisitor*)` 负责把 `Klass` 自身持有的引用暴露给 GC；新增字段时必须同步更新。
- **MRO/C3 与属性查找**:
  - `Klass::supers_` 语义上保存父类 `PyTypeObject` 列表；`OrderSupers()` 用 C3 线性化生成 `mro_`，并同时写入 `klass_properties_["mro"]`（见 [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc#L173-L199)）。
- 解释器 call-shape 快路会优先走 `GetAttrForCall`，在命中 `PyFunction/PyNativeFunction` 时直接返回裸函数并通过 out 参数回传 `self`，避免分配 `MethodObject`（见 [py-object-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object-klass.cc#L178-L219)）。
- **Handle/Tagged 转换约定**:
  - `Handle<T>::cast` 依赖 `T::cast(...)` 做断言；若某类型未提供该断言函数，需要在 T 类型的 .h 及 .cc 文件中分别补充 `Tagged<T> T::cast(Tagged<PyObject> object)` 方法的定义与实现。
- **容器 cast 语义约定（重要）**：
  - 对 `list/str/tuple/dict` 这类“已建立 native layout 继承链”的内建容器，`T::cast` 的断言语义采用 like（布局兼容）而不是 exact（klass 全等）。
  - 判定 API 统一使用 `IsPyXxx`（like）与 `IsPyXxxExact`（exact）。
  - checker 模块边界：声明/实现位于 `src/objects/object-checkers.h/.cc`；类型基建 list 位于 `src/objects/object-type-list.h`；`py-object.h` 通过 include 暴露 checker API，`py-object.cc` 不再承载 checker 定义。
  - 原因是原生子类与基类共享一致的 C++ 内存布局；差异主要在 `klass`（行为分派/方法集合），不是对象实体字段排布，因此在 C++ 层按基类容器视图访问是安全的。
  - 该约定借鉴 V8 的“按 instance type / 类型区间判定”思路：类型检查服务于“可安全执行的表示层操作”，而不强制等同于“元类身份完全一致”。
  - `IsPyString/IsPyList/IsPyDict/IsPyTuple` 默认是 like；若某调用点确实需要“exact 类型语义”，必须显式使用 `IsPyStringExact/IsPyListExact/IsPyDictExact/IsPyTupleExact`。
- **对象布局关键约束**:
  - `MarkWord` 必须位于对象内存布局起始位置（见 `PyObject` 的字段布局约束），GC 与类型信息读取依赖它。
  - `MarkWord` 语义：要么保存 `Klass` 指针，要么在 GC 期间保存 forwarding 地址（tag 为 `0b10`）。
- **Tagged 与 Smi 编码**:
  - 统一使用 `Tagged<T>` 表达“可能是堆对象，也可能是 Smi”的引用语义。
  - Smi：`Address` 末位为 `1`，值编码为 `SmiToAddress(v) = (v << 1) | 1`（见 `include/saauso-internal.h`）。
  - 堆对象：依赖对齐保证低位为 `0`（当前对齐为 8 字节，`kObjectAlignmentBits = 3`），不使用额外 tag 位。
  - 重要：`Tagged<T>::Cast` 的语义相当于 `reinterpret_cast`，只能在“你确信类型正确”时使用；类型断言通常由 `T::Cast(...)` 或 `IsPyXxx(...)` 承担。

## 3. 内存管理 (`src/heap`)

- **垃圾回收 (GC)**: 当前实现以新生代 Scavenge 为主。
  - **NewSpace**: 复制算法（Eden + Survivor，Flip）。注意：当前 `NewSpace::Contains()` 仅判断 Eden；涉及空间判定时优先使用 `Heap::InNewSpaceEden()` / `Heap::InNewSpaceSurvivor()` 或直接对 `eden_space()/survivor_space()` 做 Contains。
  - **OldSpace**: 地址段已预留，但 `AllocateRaw/Contains` 尚未实现（当前会触发断言）；MVP 主路径应避免 OldSpace 分配，老生代回收仍在 TODO。
  - **MetaSpace**: 永久区（例如 `None/True/False` 等全局单例与 `Klass` 相关数据）。
- **根集合 (Roots)**:
  - `Isolate::klass_list_`：遍历各 `Klass` 内部引用（通常由各 `Klass::PreInitialize()` 注册）。
  - `HandleScopeImplementer`：遍历所有 handle blocks 内引用。
  - `Interpreter`：解释器内部持有的引用也会被遍历（见 `Heap::IterateRoots`）。
  - `ExceptionState`：pending exception 属于 GC root（见 `Heap::IterateRoots`）。
  - `ModuleManager`：遍历模块系统中持有的引用（`sys.modules/sys.path`），保证 import 引入的模块与路径在 minor GC 下可达。
  - `StringTable` 当前把常用字符串驻留在 MetaSpace，`Heap::IterateRoots` 暂未开放其遍历入口。
  - Python 运行时根（更完整的栈帧/全局变量等）仍在 TODO（见 `Heap::IterateRoots`）。
- **Remembered set / Write barrier（现状）**:
  - `Heap::RecordWrite()` 与 `remembered_set_` 已实现雏形，用于记录 “Old -> New” 跨代写入。
  - 目前通过 `WRITE_BARRIER` 宏强制为空、且 `IterateRoots()` 内的 `IterateRememberedSet()` 调用被禁用，MVP 不依赖该机制。
- **句柄系统 (`src/handles`)**:
  - `Handle<T>` 是“会被 GC 移动/回收的对象”的栈上安全持有方式；`Tagged<T>` 更接近裸指针语义。
  - `HandleScope` 管理 handle 生命周期；跨 scope 返回时应当使用语义更加明确的 `EscapableHandleScope`。
  - 永久区对象（例如 `Isolate::py_none_object()` / `py_true_object()` / `py_false_object()`）分配在 `kMetaSpace`，不会被回收移动，通常可以直接用 `Tagged<T>` 返回/保存。
- **TODO**: 如果虚拟机的基础功能开发完毕后仍有多余时间，再进行分代式 GC 的开发。当前的目标是实现一个仅含有新生代 scavenge GC 的 MVP 版本。

## 4. 执行与隔离 (`src/execution`)

- **Isolate**: 独立的虚拟机实例容器，封装堆 (Heap)、对象工厂 (Factory)、句柄作用域实现 (HandleScopeImplementer)、解释器 (Interpreter)、模块管理器 (ModuleManager)、字符串表 (StringTable)、各内建 `Klass` 指针，以及全局单例（`None/True/False`）。
- **Current 绑定模型**: `Isolate::Current()` 通过 `thread_local` 保存当前线程绑定的 Isolate；进入/退出必须使用 `Isolate::Scope`（或显式 `Enter/Exit`），禁止手动设置 Current。
- **多线程访问控制**: `Isolate::Locker` 基于递归互斥锁保证同一时刻仅一个线程访问某个 Isolate（Isolate 级别的“GIL”语义）。
- **关键不变量**：
  - `Isolate::Scope` 的生命周期必须完全被 `Isolate::Locker` 覆盖（若使用多线程访问控制）。
  - `Isolate::Dispose()` 的前置条件是 `entry_count_ == 0`（必须先正确退出所有 scope）。
- **初始化流程（关键）**:
  - 预初始化所有 `Klass`：调用 `PreInitialize()`（完成 vtable 初始填充、把 Klass 注册到 `klass_list_` 等）。
  - 初始化 `StringTable`。
  - 创建全局单例 `None/True/False`（这些对象需要在大量初始化逻辑之前就可用）。
  - 正式初始化所有 `Klass`：`Initialize()`（常见动作：创建类字典、C3/MRO、绑定 type object 等）。
  - 初始化 `Interpreter`（提供字节码执行入口）。
  - 初始化 `ModuleManager`（负责 `sys.modules/sys.path` 与 import 入口）。
  - 构建并挂载 `builtins` 字典（`BuiltinBootstrapper`）。
- **销毁流程（关键）**:
  - 先 `Finalize()` 所有 `Klass`（反向释放与清理元数据）。
  - 再依次销毁 `StringTable`、`Interpreter`、`ModuleManager`、`HandleScopeImplementer`、最后销毁 `Heap` 与 `mutex`，并清空 `klass_list_` 与 `None/True/False` 引用（当前实现不保证严格与初始化逆序一致）。
- **全局运行时初始化**: `saauso::Saauso::{Initialize,Dispose}` 位于 `src/init/`，用于嵌入式 CPython312 编译器前端的 Setup/TearDown。

## 5. Runtime helpers (`src/runtime`)

- **builtins（内建函数）**: 实现在 `src/builtins/`，并由 `BuiltinBootstrapper` 在 `Isolate` 初始化阶段构建后挂到 `isolate->builtins()`（而非放在 `src/runtime/` 或由 `Interpreter` 自身持有）。
- **runtime**: 可复用的运行时 helper（例如 unpack iterable、扩展 list 等）。上层类型实现优先复用此处 helper，减少重复迭代/拆包逻辑。
- **exec 执行入口**：`Runtime_ExecutePyCodeObject/Runtime_ExecutePythonSourceCode` 负责在指定 `globals/locals` 字典中执行代码；当 `globals` 缺少 `__builtins__` 时会自动注入当前 `Isolate` 持有的 builtins（对齐 CPython 行为）。
- **StringTable**: 常用字符串常量池（通常在 `kMetaSpace` 分配字符串对象，避免被 GC 移动）。
  - **文件组织**：按“通用语义 + 对象域 helper”拆分为 `runtime-*.{h,cc}`；除 `truthiness/iterable/intrinsics/conversions/reflection/exec/string` 等通用模块外，当前还稳定包含 `runtime-py-dict/list/tuple/string/function/smi/type-object` 等对象域入口，各 header 只暴露本领域的对外接口，避免出现“万能头文件”导致的编译期耦合。
  - **推荐放置规则**：
    - 字符串相关高层语义（split/join/`str(x)` 转换）优先放在 `runtime-py-string.cc`。
    - 容器与对象族的高层 Python 语义优先放在对应 `runtime-py-*.cc`，避免把跨类型流程重新塞回 `py-xxx.cc` 实体类。
    - iterable 通用语义放在 `runtime-iterable.cc`。
    - 反射/MRO/property find/magic method invoke 放在 `runtime-reflection.cc`。
    - exec/source/code 执行入口放在 `runtime-exec.cc`。

## 6. 字节码解释器 (`src/interpreter`)

- **Interpreter**：字节码执行入口与跨语言调用入口；负责维护当前栈帧链、异常展开状态与 computed-goto 的 dispatch table。`builtins` 字典本身归属 `Isolate`，Interpreter 按需读取。
- **builtins 字典（行为对齐用）**：
  - builtins 字典由 `BuiltinBootstrapper` 统一构建并挂到 `Isolate`（避免 Interpreter 构造函数膨胀），见 [builtin-bootstrapper.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.h)。
- 同时会注入最小异常类型对象体系：`BaseException/Exception` 以及 `TypeError/RuntimeError/ValueError/IndexError/KeyError/NameError/AttributeError/ZeroDivisionError/ImportError/ModuleNotFoundError/StopIteration` 等。
- **computed-goto dispatcher**：`Interpreter::EvalCurrentFrame()` 使用 256 槽 dispatch table（未知 opcode 默认跳到 `unknown_bytecode`），每个 handler 内优先创建 `HandleScope`，避免 GC 移动导致悬垂引用。
- **FrameObject（栈帧）**：
  - 保存 `stack/fast_locals/locals/globals/consts/names/code_object` 等字段，并在 `Iterate(ObjectVisitor*)` 中暴露为 GC roots。
  - **参数绑定与默认值**：函数调用的形参绑定主要在 `FrameObjectBuilder::BuildSlowPath/BuildFastPath` 中完成；支持位置参数、关键字参数、默认参数回填，以及 `*args/**kwargs` 的打包与注入。
- **异常表（exception table）**：
  - 解释器侧引入 `ExceptionTable` 来解析并线性扫描 `co_exceptiontable`（CPython 3.11+ 的 varint 编码格式），用于查找 try/except 的 handler，见 [exception-table.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/exception-table.cc)。
  - `PyCodeObject` 保存 `exception_table_` 并在 GC iterate 中显式遍历，`.pyc` 解析器也会读取该字段，见 [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h) 与 [cpython312-pyc-file-parser.cc](file:///e:/MyProject/S.A.A.U.S.O/src/code/cpython312-pyc-file-parser.cc)。
  - 当前解释器已实现 `pending_exception_unwind` 主路径：命中 handler 时恢复栈深并跳转，未命中时跨 frame 回溯，属于现有异常语义的核心组成部分。
- **调用约定（面向实现而非语义保证）**：
  - `CALL + KW_NAMES`：按 CPython3.12 “双槽位调用协议”组织 operand stack（见下文），`KW_NAMES` 提供 `kwarg_keys`；`CALL_FUNCTION_EX` 处理 `f(*args, **kwargs)`；`DICT_MERGE` 处理 kwargs dict 合并。
  - `CALL_INTRINSIC_1`：当前至少支持 `INTRINSIC_LIST_TO_TUPLE` 与 `INTRINSIC_IMPORT_STAR`；新增 intrinsic 时需同步检查 dispatch、错误传播与解释器回归测试。

### 6.1. CPython3.12 Call 双槽位调用协议（关键）

CPython 3.12 为了优化“方法调用（obj.meth(...)）”的热点路径，引入了一个在字节码层面可表达的约定：把“是否需要隐式注入 self”编码到 operand stack 的两个前缀槽位中，从而让 `CALL` 在运行时只做非常轻量的栈调整。

**核心栈布局**

对任意一次调用，在执行 `CALL oparg` 前，operand stack 的尾部应满足：

```
..., method_or_NULL, callable_or_self, arg1, arg2, ..., argN
```

- `N == oparg`
- `method_or_NULL == NULL`：表示“普通调用”，此时 `callable_or_self` 就是被调用对象（callable）
- `method_or_NULL != NULL`：表示“方法调用形态”，此时
  - `callable_or_self` 实际存放的是 `self`
  - `method_or_NULL` 才是真正要调用的函数对象（裸 function / 裸 native function）

随后 `CALL` 的语义等价于（伪代码）：

```
args = stack_pointer - oparg
callable = stack_pointer[-(1 + oparg)]
method = stack_pointer[-(2 + oparg)]
if (method != NULL) {
  callable = method
  args--          // 把 self 纳入 args 视图
  total_args++
}
```

这套协议的关键在于：call-shape 的 `LOAD_ATTR` 可以直接把 `(method,self)` 放到栈上，避免创建中间的 `PyMethodObject`。

**哪些指令负责“生产/消费”这两个前缀槽位**

- 生产端（把双槽位布局准备好）：
  - `PUSH_NULL`：压入一个 NULL 哨兵
  - `LOAD_GLOBAL`：当 `oparg & 1 != 0` 时，先压 NULL 再压 value
  - `LOAD_ATTR`：当 `oparg & 1 != 0` 时，压入 `(method_or_NULL, self_or_value)` 的双槽位形态
- 消费端（按双槽位布局调用）：
  - `CALL`：消费 `method_or_NULL`/`callable_or_self` 两槽位 + `oparg` 个参数
  - `CALL_FUNCTION_EX`：同样消费两槽位，再消费 `args_obj` 与可选 `**kwargs`
- `KW_NAMES`：不改栈布局；它只是把关键字参数 key tuple 暂存起来供下一条 `CALL` 消费。

**S.A.A.U.S.O 中的具体实现（请以仓库现状为准）**

- 栈布局与字节码 handler：
  - `src/interpreter/interpreter-dispatcher.cc`
    - `PushNull`：真实压入 `Tagged<PyObject>::null()`
    - `LoadGlobal`：实现 `op_arg & 1` 的 NULL 前缀压栈
    - `LoadAttr`：当 `op_arg & 1`（call-shape）时走 `PyObject::GetAttrForCall`，直接压栈 `(method_or_NULL, self_or_value)`，不创建 `MethodObject`
    - `Call` / `CallFunctionEx`：消费双槽位；若 `method != NULL`，则直接以 `callable=method`、`host=self` 进入调用路径（不会重建 `MethodObject`）
    - `KwNames`：设置 `kwarg_keys_`，由 `ReleaseKwArgKeys()` 做“一次性消费并清空”
- “不分配 MethodObject 的方法查找”：
  - `src/objects/py-object.{h,cc}`：`PyObject::GetAttrForCall(self, name, self_or_null)` 返回“可调用对象”，并通过 out-param 返回是否需要注入的 `self`
  - `src/objects/py-object-klass.{h,cc}`：`PyObjectKlass::Generic_GetAttrForCall` 在命中 `PyFunction/PyNativeFunction` 时直接返回裸函数 + `self_or_null=self`；命中实例 dict 时不绑定；其它情况回退到普通 `GetAttr` 语义
- host 注入与调用路径（与双槽位协议的衔接点）：
  - `src/interpreter/interpreter.{h,cc}` / `src/interpreter/interpreter-dispatcher.cc`
    - 调用入口统一走 `Interpreter::InvokeCallable*`，其核心是把 `host` 传给 `FrameObjectBuilder::PrepareForFunction`/`BuildFastPath`/`BuildSlowPath`
    - `FrameObjectBuilder` 若 `host` 非空，会将其写入 `localsplus[0]` 作为隐式 `self`

**迁移/扩展注意事项**

- 如果未来引入更完整的 descriptor/`__get__` 协议，需要同步扩展 `GetAttrForCall` 的“是否可拆成 (method,self)”判定规则；目前仅对 `PyFunction/PyNativeFunction` 做了特判优化。

## 7. 代码加载 / 前端 (`src/code`)

- **Compiler**：编译入口，提供 `CompileSource/CompilePyc` 等 API。`CompileSource` 的默认实现为“嵌入式 CPython312 编译成 pyc bytes，再解析为 `PyCodeObject`”。
- **cpython312-pyc-file-parser**：解析 CPython 3.12 `.pyc` 并构建 `PyCodeObject`。
- **cpython312-pyc-compiler（可选）**：通过嵌入式 CPython 3.12 生成 `.pyc`（对应 GN 目标 `saauso_cpython312_compiler`）。
- **BinaryFileReader**：二进制读取工具，位于 `src/utils/`，供 pyc parser 等模块复用。

## 8. 模块系统（import，`src/modules`）

- **分层职责（按“语义编排/定位/加载/状态”拆分）**：
  - `ModuleManager`：模块系统门面与状态持有者，维护 `sys.modules`（dict）与 `sys.path`（list），并向解释器暴露统一入口 `ImportModule(...)`（见 [module-manager.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.h)）。
  - `ModuleImporter`：导入语义编排层，负责 dotted-name 分段导入、`fromlist` 语义、父子模块绑定，以及在 `level` 非 0 时配合相对导入名解析（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
  - `ModuleNameResolver`：相对导入名解析器，基于 `globals["__package__"]` / `globals["__name__"]` 与是否为 package（存在 `globals["__path__"]`）决定相对导入基准，并处理越界/无父包等错误（见 [module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)）。
  - `ModuleFinder`：纯文件系统定位层，只负责在给定 search path 中查找 module/package 的位置，不执行模块体（见 [module-finder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-finder.cc)）。
  - `ModuleLoader`：加载与执行层，负责创建 `PyModule`、初始化 `__name__/__package__/__file__/__path__` 等关键字段，并执行模块体（source 或 pyc）；同时负责 builtin 模块的创建（见 [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)）。
  - `BuiltinModuleRegistry`：内建模块注册表，仅维护 name → init 函数映射，供 `ModuleLoader` 优先命中（见 [builtin-module-registry.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module-registry.cc)）。
  - `ModuleUtils`：模块系统通用工具（模块名合法性、package 判定、`__path__` 提取等），用于减少 ModuleImporter/Loader 的重复样板代码（见 [module-utils.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-utils.h)）。
- **sys 状态初始化**：
  - `sys.modules` 初始为空 dict；`sys.path` 初始为 `['.']`（见 [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)）。
- **查找与加载规则（文件系统）**：
  - 文件系统查找优先级：对每个 base path，先尝试 package（`<base>/<name>/__init__.py` → `<base>/<name>/__init__.pyc`），再尝试 module（`<base>/<name>.py` → `<base>/<name>.pyc`），并遵循“`.py` 优先、`.pyc` 兜底”（见 [module-finder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-finder.cc)）。
  - 不默认扫描 `__pycache__/<name>.*.pyc` 这类 tagged 缓存路径；embedder 若只分发字节码，建议直接提供同目录的 `<name>.pyc` 或 `<pkg>/__init__.pyc`。
  - 搜索路径来源：导入顶层段使用 `sys.path`；导入子模块使用 `package.__path__`，若父模块不是 package 则 fail-fast（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
- **关键语义点（以仓库现状为准）**：
  - 父子模块绑定：导入 `pkg.sub` 后会把子模块写入父模块 dict（`pkg.sub = <module>`），保证 Python 层可见（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
  - `fromlist` 决定 `IMPORT_NAME` 返回值：当 `fromlist` 为空且导入 dotted-name 时返回顶层包，否则返回最后导入的模块对象（见 [module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)）。
  - 循环导入：CPython 语义通常要求“执行模块体前就将 module 放入 sys.modules”。当前实现把模块写入 `sys.modules` 的时机在执行模块体之后，因此循环导入行为不保证对齐 CPython，后续需要专门补齐（见 [module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)）。
- **内建模块（builtin）**：
  - 内建模块通过注册表机制优先命中（见 [builtin-module.h](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-module.h)）。当前内建模块列表含 `sys/math/random/time`；`sys.modules/sys.path` 指向 `ModuleManager` 持有对象（见 [builtin-sys-module.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-sys-module.cc)）。

## 9. Embedder API

阅读与落地建议（AI 与人类程序员通用）：
- 入门与使用约定：先读 `docs/Embedder-API-User-Guide.md`（概念、示例、错误处理、限制）。
- 架构与边界：再读 `docs/Embedder-API-Architecture-Design.md`（分层、生命周期、互调链路、源码索引）。
- 代码导航顺序：
  1. 公共 ABI：`include/saauso.h` 与 `include/saauso-*.h`
  2. 桥接层实现：`src/api/`
  3. VM 侧关键互调：`src/runtime/runtime-py-function.cc`、`src/objects/py-function-klass.cc`
  4. 示例与回归：`samples/`、`test/embedder/`