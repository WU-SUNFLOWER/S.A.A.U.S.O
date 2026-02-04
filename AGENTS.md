# S.A.A.U.S.O 项目 - AI 助手指南

本文档概述了 S.A.A.U.S.O 项目的架构决策、代码标准和开发工作流。AI 助手需要扮演一位资深 C++ 工程师，在生成代码时应参考本文档，以确保与现有代码库保持一致。

## 0. 给 AI 的执行规则（必读）

### 0.1. 执行优先级与冲突处理
- **规则优先级（从高到低）**：
  1. 仓库现有实现与“能编译/能链接/能运行单测”的事实
  2. 本文档（AGENTS.md）
  3. Google C++ Style Guide
  4. 通用最佳实践
- **冲突处理流程**：
  1. 先在仓库中搜索同类代码与既有模式（优先保持局部一致性）。
  2. 若仍存在多种可行实现，选择更符合本仓库架构约束、且对现有代码扰动更小的方案。
  3. 若约束仍不明确，优先保证行为正确与可测试性，再逐步收敛风格。
- **用户指令优先**：当用户的最新明确指令与本文档冲突时，以用户指令为准；但应在交付说明中点出偏离点与潜在风险，并尽量把偏离范围限制在最小。

### 0.2. Top 10 必守规则（AI Checklist）
1. 修改前先在仓库中搜索同类实现与既有模式，再决定具体写法（见 0.1）。
2. 禁止在接口上传递 `PyObject*`；对外暴露与内部调用使用 `Tagged<PyObject>` 或 `Handle<PyObject>`（见 6.1）。
3. 栈上持有 GC-able 对象必须使用 `Handle<T>`；跨 `HandleScope` 返回必须使用 `EscapableHandleScope::Escape`（见 6.1）。
4. 分配 `PyObject` 派生对象禁止使用 `new`；必须通过 `Isolate::Current()->heap()->Allocate<T>(...)` 获取地址并显式初始化字段与 `SetKlass`（见 6.2）。
5. 不依赖构造函数写默认值：`Allocate/AllocateRaw` 不清零且不调用构造函数，默认值应在工厂函数中手工写入（见 6.2）。
6. 新增/重写 `Klass::vtable_` 的 slot 时必须显式指向默认实现，或确保所有调用点对 `nullptr` 可处理（见 3.1）。
7. `instance_size` 必须为“不可触发 GC”的纯计算；`iterate` 必须遍历对象内全部 `Tagged<PyObject>` 引用字段（见 3.1、6.3）。
8. `src/utils/` 严禁依赖虚拟机上层能力；不确定时先查同目录既有代码并保持依赖方向单向（见 2）。
9. 所有内部代码必须位于 `namespace saauso::internal`，并遵循第 4 章的命名与风格规范。
10. 新增单元测试文件后必须同步加入 `test/unittests/BUILD.gn` 的 `sources` 列表（见 5.3）。

## 1. 项目概览
S.A.A.U.S.O 是一款高性能 Python 虚拟机，旨在兼容 CPython 字节码。
- **开发者**: WU-SUNFLOWER（Computer Science专业本科大四学生）
  - GitHub主页：https://github.com/WU-SUNFLOWER
  - 个人博客：https://juejin.cn/user/3544481220008744
- **开发目的**：
 - 这是WU-SUNFLOWER的本科毕业设计。
 - 他希望通过开发这个项目，在顺利拿到CS专业学士学位的同时，帮助自己提升对编程语言虚拟机的理解和工程实现能力。为深入阅读和学习工业级虚拟机（如HotSpot JVM、V8、CPython）的源代码打下坚实的基础。
 - 此外，WU-SUNFLOWER希望能够将本项目写进求职简历的"个人作品"栏目，增强他在未来通过社会招聘跳槽时的市场竞争力。
- **距离WU-SUNFLOWER提交毕业设计作品的剩余时间**: 大约2个月不到
- **语言**: C++20。
- **构建系统**: GN (Generate Ninja) + Ninja。
- **测试框架**: Google Test。
- **辅助调试工具**: LLDB、AddressSanitizer、UBSanitizer

### 1.1. 当前实现进度（以仓库现状为准）
- **已具备**：
  - 基础对象系统：`PyObject/Klass/VTable`、`PyTypeObject`，以及若干内建类型（`int(Smi)`/`float`/`str`/`list`/`tuple`/`dict`/`bool`/`None`）。
  - 句柄系统：`HandleScope` + `HandleScopeImplementer`，以及长期句柄 `Global<T>`（会被 GC 扫描并在 minor GC 后更新）。
  - 堆与 GC：按 `NewSpace/OldSpace/MetaSpace` 分区；MVP 以新生代 scavenge 为主（老生代回收仍在 TODO）。
  - 字节码解释器：基于 CPython 3.12 字节码模型的初版执行引擎（computed-goto dispatch）、栈帧与参数绑定、基础 builtins（`print/len/isinstance/build_class` 等），并注入若干内建类型名（`object/int/str/list/bool/dict/tuple`）与单例（`True/False/None`）。
  - `.pyc` 前端：CPython 3.12 `.pyc` 解析器与（可选）嵌入式 CPython 3.12 编译器前端目标。
- **尚未重点覆盖/仍在 TODO（非穷尽）**：
  - 异常体系（当前大量错误以 `stderr + exit(1)` 方式处理）。
  - 老生代回收，以及分代式 GC 所需的 remembered set / write barrier（已实现雏形，但当前通过宏与 root-iterate 路径整体禁用，MVP 不做跨代引用处理）。
  - 更完整的 Python 语义：import/module、class 语义细节、生成器/协程等。

## 2. 仓库结构速览
- `include/`：对外/跨模块共享的基础定义（例如 `Address`、Smi/对齐等）。
- `src/code/`：`.pyc` 解析与前端工具（`BinaryFileReader`、`cpython312-pyc-file-parser`、`cpython312-pyc-compiler` 等）。
- `src/interpreter/`：字节码解释器（bytecode dispatcher、`FrameObject` 栈帧、参数归一化与调用入口）。
- `src/objects/`：对象系统（`PyObject`、`Klass`、各内建对象与其 `*-klass`）。
- `src/handles/`：句柄系统（`Handle`/`HandleScope`/`HandleScopeImplementer`）、长期句柄 `Global<T>` 与 `Tagged<T>`。
- `src/heap/`：堆与空间（`NewSpace`/`OldSpace`/`MetaSpace`）以及新生代 GC（Scavenge）。
- `src/runtime/`：运行时容器（`Isolate`）与隔离性/多线程访问控制（`thread_local` Current + `Isolate::Scope/Locker`）。
- `src/utils/`：通用工具（对齐/内存/小型容器等）。**该目录下的代码严禁依赖和调用虚拟机的任何上层能力！**
- `test/unittests/`：基于 GTest 的单元测试。
- `BUILD.gn`：根目标定义（当前主要目标：`vm`、`ut`）。
- `build/`：编译配置与工具链（Clang/LLD，支持 `is_debug`/`is_asan`）。
- `testing/gtest/`：项目内置的 GTest GN 封装目标（供 `ut` 依赖）。
- `third_party/`：第三方代码（例如 `googletest` 上游源码、`rapidhash`）。

## 2.1. 建议阅读路线（快速上手）
- 生命周期与入口：从 [main.cc](file:///e:/MyProject/S.A.A.U.S.O/src/main.cc) 看初始化/创建 Isolate/执行 `.pyc`。
- 运行时与初始化顺序：读 [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/isolate.cc)（`Init/InitMetaArea/TearDown`）。
- 字节码执行主循环：读 [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc)（computed-goto handlers）。
- 调用与参数绑定：读 [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc) 与 [frame-object-builder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object-builder.cc)。
- 对象模型与属性查找：读 [py-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.cc) 与 [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)。
- 堆与新生代 GC：读 [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc) / [spaces.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.cc) / [scavenge-visitor.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/scavenge-visitor.cc)。

## 3. 架构指南

本项目的架构设计深度借鉴了 V8 和 HotSpot。

### 3.1. 对象系统 (`src/objects`)
- **PyObject**: 所有堆对象的基类。
- **Klass**: 表示对象的类型/元信息（类似于 V8 的 Map 或 HotSpot 的 Klass）。数据（`PyObject`）与行为（`Klass`）严格分离。
- **Object**: 仅作为 C++ 侧的轻量基类（见 `src/objects/objects.h`），不要与 Python 堆对象基类 `PyObject` 混淆。
- **VirtualTable (slot 表)**:
  - `Klass::vtable_` 保存各类操作的函数指针，优先使用以 `Handle<PyObject>` 为参数的 SAFE 版本，避免 GC 导致悬垂引用。
  - 新增/重写 slot 时：要么显式指向默认实现，要么确保所有调用点在 `nullptr` 时可处理（否则会空函数指针调用崩溃）。
  - `instance_size` 必须为“不可触发 GC”的纯计算；`iterate` 必须遍历对象内部所有 `Tagged<PyObject>` 引用字段。
  - `getattr` 采用三参形式：`getattr(self, name, is_try)`。当 `is_try=true` 时属性 miss 返回 null；当 `is_try=false` 时属性 miss 走 `AttributeError + exit(1)`（当前异常体系尚未完善）。
  - 比较与 `contains` 等热点 slot 在 VirtualTable 层以 C++ `bool` 作为 canonical 返回值；Python 语义需要 `True/False` 时由上层 wrapper 转换。
- **Klass 也是 GC Root**:
  - `Klass::Iterate(ObjectVisitor*)` 负责把 `Klass` 自身持有的引用暴露给 GC；新增字段时必须同步更新。
- **MRO/C3 与属性查找**:
  - `Klass::supers_` 语义上保存父类 `PyTypeObject` 列表；`OrderSupers()` 用 C3 线性化生成 `mro_`，并通常同时写入 `klass_properties_["mro"]`。
  - 默认 getattr 流程：先沿 MRO 查 `__getattr__`，再查实例 dict（仅 heap object），最后沿 MRO 查类 dict；命中函数时可按“普通语义”包装成 `MethodObject(func, self)`，但解释器的 call-shape 路径会优先走 `GetAttrForCall` 来避免分配 `MethodObject`（见 3.4.1）。
- **Handle/Tagged 转换约定**:
  - `Handle<T>::cast` 依赖 `T::cast(...)` 做断言；若某类型未提供该断言函数，需要在T类型的.h及.cc文件中分别补充`Tagged<T> T::cast(Tagged<PyObject> object)`方法的定义与实现。
- **对象布局关键约束**:
  - `MarkWord` 必须位于对象内存布局起始位置（见 `PyObject` 的字段布局约束），GC 与类型信息读取依赖它。
  - `MarkWord` 语义：要么保存 `Klass` 指针，要么在 GC 期间保存 forwarding 地址（tag 为 `0b10`）。
- **Tagged 与 Smi 编码**:
  - 统一使用 `Tagged<T>` 表达“可能是堆对象，也可能是 Smi”的引用语义。
  - Smi：`Address` 末位为 `1`，值编码为 `SmiToAddress(v) = (v << 1) | 1`（见 `include/saauso-internal.h`）。
  - 堆对象：依赖对齐保证低位为 `0`（当前对齐为 8 字节，`kObjectAlignmentBits = 3`），不使用额外 tag 位。
  - 重要：`Tagged<T>::Cast` 的语义相当于 `reinterpret_cast`，只能在“你确信类型正确”时使用；类型断言通常由 `T::Cast(...)` 或 `IsPyXxx(...)` 承担。

### 3.2. 内存管理 (`src/heap`)
- **垃圾回收 (GC)**: 当前实现以新生代 Scavenge 为主。
  - **NewSpace**: 复制算法（Eden + Survivor，Flip）。注意：当前 `NewSpace::Contains()` 仅判断 Eden；涉及空间判定时优先使用 `Heap::InNewSpaceEden()` / `Heap::InNewSpaceSurvivor()` 或直接对 `eden_space()/survivor_space()` 做 Contains。
  - **OldSpace**: 已划分空间，但回收逻辑仍在 TODO（目前可能出现老生代 OOM 即退出）。
  - **MetaSpace**: 永久区（例如 `None/True/False` 等全局单例与 `Klass` 相关数据）。
- **根集合 (Roots)**:
  - `Isolate::klass_list_`：遍历各 `Klass` 内部引用（通常由各 `Klass::PreInitialize()` 注册）。
  - `HandleScopeImplementer`：遍历所有 handle blocks 内引用。
  - `Interpreter`：解释器内部持有的引用也会被遍历（见 `Heap::IterateRoots`）。
  - `StringTable` 当前把常用字符串驻留在 MetaSpace，`Heap::IterateRoots` 暂未开放其遍历入口。
  - Python 运行时根（更完整的栈帧/全局变量等）仍在 TODO（见 `Heap::IterateRoots`）。
- **Remembered set / Write barrier（现状）**:
  - `Heap::RecordWrite()` 与 `remembered_set_` 已实现雏形，用于记录 “Old -> New” 跨代写入。
  - 目前通过 `WRITE_BARRIER` 宏强制为空、且 `IterateRoots()` 内的 `IterateRememberedSet()` 调用被禁用，MVP 不依赖该机制。
- **句柄系统 (`src/handles`)**:
  - `Handle<T>` 是“会被 GC 移动/回收的对象”的栈上安全持有方式；`Tagged<T>` 更接近裸指针语义。
  - `HandleScope` 管理 handle 生命周期；跨 scope 返回时应当使用语义更加明确的`EscapableHandleScope`。
  - 永久区对象（例如 `Isolate::py_none_object()` / `py_true_object()` / `py_false_object()`）分配在 `kMetaSpace`，不会被回收移动，通常可以直接用 `Tagged<T>` 返回/保存。
- **TODO**: 如果虚拟机的基础功能开发完毕后仍有多余时间，再进行分代式GC的开发。**当前的目标是实现一个仅含有新生代scavenge gc的MVP版本**！

### 3.3. 运行时 (`src/runtime`)
- **Isolate**: 独立的虚拟机实例容器，封装堆 (Heap)、句柄作用域实现 (HandleScopeImplementer)、解释器 (Interpreter)、字符串表 (StringTable)、各内建 `Klass` 指针，以及全局单例（`None/True/False`）。
- **Current 绑定模型**: `Isolate::Current()` 通过 `thread_local` 保存当前线程绑定的 Isolate；进入/退出必须使用 `Isolate::Scope`（或显式 `Enter/Exit`），禁止手动设置 Current。
- **多线程访问控制**: `Isolate::Locker` 基于递归互斥锁保证同一时刻仅一个线程访问某个 Isolate（Isolate 级别的“GIL”语义）。
- **初始化流程（关键）**:
  - 预初始化所有 `Klass`：`InitializeVTable()` + `PreInitialize()`（完成 vtable 填充、把 Klass 注册到 `klass_list_` 等）。
  - 初始化 `StringTable`。
  - 创建全局单例 `None/True/False`（这些对象需要在大量初始化逻辑之前就可用）。
  - 正式初始化所有 `Klass`：`Initialize()`（常见动作：创建类字典、C3/MRO、绑定 type object 等）。
- **销毁流程（关键）**:
  - 先 `Finalize()` 所有 `Klass`（反向释放与清理元数据）。
  - 再依次销毁 `StringTable`、`Interpreter`、`HandleScopeImplementer`、最后销毁 `Heap` 与 `mutex`，并清空 `klass_list_` 与 `None/True/False` 引用。
- **Interpreter**: 当前字节码执行与内建函数注册入口（内建函数实现在 `native-functions.*`）。
- **StringTable**: 常用字符串常量池（通常在 `kMetaSpace` 分配字符串对象，避免被 GC 移动）。
- **隔离性**: 主路径已迁移为 Isolate 架构；需要全局状态时，优先通过 `Isolate::Current()` 获取（而不是引入新的静态全局）。

### 3.4. 字节码解释器 (`src/interpreter`)
- **Interpreter**：字节码执行入口与跨语言调用入口；负责维护 `builtins`、当前栈帧链、以及 computed-goto 的 dispatch table。
- **builtins 字典（行为对齐用）**：
  - 除 `print/len/isinstance/build_class` 外，还会注入 `object/int/str/list/bool/dict/tuple` 的 type 对象、`True/False/None` 单例，并把 `builtins` 自身注册进 builtins dict（自引用）。
- **computed-goto dispatcher**：`Interpreter::EvalCurrentFrame()` 使用 256 槽 dispatch table（未知 opcode 默认跳到 `unknown_bytecode`），每个 handler 内优先创建 `HandleScope`，避免 GC 移动导致悬垂引用。
- **FrameObject（栈帧）**：
  - 保存 `stack/fast_locals/locals/globals/consts/names/code_object` 等字段，并在 `Iterate(ObjectVisitor*)` 中暴露为 GC roots。
  - **参数绑定与默认值**：函数调用的形参绑定主要在 `FrameObjectBuilder::BuildSlowPath/BuildFastPath` 中完成；支持位置参数、关键字参数、默认参数回填，以及 `*args/**kwargs` 的打包与注入。根栈帧由 `FrameObjectBuilder::BuildRootFrame` 创建。
- **调用约定（面向实现而非语义保证）**：
  - `CALL + KW_NAMES`：按 CPython3.12 “双槽位调用协议”组织 operand stack（见下文），`KW_NAMES` 提供 `kwarg_keys`；`CALL_FUNCTION_EX` 处理 `f(*args, **kwargs)`；`DICT_MERGE` 处理 kwargs dict 合并。
  - 当前大量错误处理为 `stderr + exit(1)`，尚未形成完整异常传播体系。

#### 3.4.1. CPython3.12 Call 双槽位调用协议（关键）

CPython 3.12 为了优化“方法调用（obj.meth(...)）”的热点路径，引入了一个在字节码层面可表达的约定：**把“是否需要隐式注入 self”编码到 operand stack 的两个前缀槽位中**，从而让 `CALL` 在运行时只做非常轻量的栈调整。

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

这套协议的关键在于：**call-shape 的 `LOAD_ATTR` 可以直接把 `(method,self)` 放到栈上，避免创建中间的 `PyMethodObject`。**

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
    - `LoadAttr`：当 `op_arg & 1`（call-shape）时走 `PyObject::GetAttrForCall`，直接压栈 `(method,function,self)` 或 `(NULL,value)`，不创建 `MethodObject`
    - `Call` / `CallFunctionEx`：消费双槽位；若 `method != NULL`，则直接以 `callable=method`、`host=self` 进入调用路径（不会重建 `MethodObject`）
    - `KwNames`：设置 `kwarg_keys_`，由 `ReleaseKwArgKeys()` 做“一次性消费并清空”
- “不分配 MethodObject 的方法查找”：
  - `src/objects/py-object.{h,cc}`：`PyObject::GetAttrForCall(self, name, self_or_null)` 返回“可调用对象”，并通过 out-param 返回是否需要注入的 `self`
  - `src/objects/klass.{h,cc}`：默认实现 `Klass::Virtual_Default_GetAttrForCall` 在命中 `PyFunction/PyNativeFunction` 时直接返回裸 function + `self_or_null=self`；命中实例 dict 时不绑定；其它情况可回退到普通 getattr 语义
- host 注入与调用路径（与双槽位协议的衔接点）：
  - `src/interpreter/interpreter.{h,cc}` / `src/interpreter/interpreter-dispatcher.cc`
    - 调用入口统一走 `Interpreter::InvokeCallable*`，其核心是把 `host` 传给 `FrameObjectBuilder::PrepareForFunction`/`BuildFastPath`/`BuildSlowPath`
    - `FrameObjectBuilder` 若 `host` 非空，会将其写入 `localsplus[0]` 作为隐式 `self`

**迁移/扩展注意事项**

- 如果未来引入更完整的 descriptor/`__get__` 协议，需要同步扩展 `GetAttrForCall` 的“是否可拆成 (method,self)”判定规则；目前仅对 `PyFunction/PyNativeFunction` 做了特判优化。

### 3.5. 代码加载 / 前端 (`src/code`)
- **BinaryFileReader**：二进制读取工具。
- **cpython312-pyc-file-parser**：解析 CPython 3.12 `.pyc` 并构建 `PyCodeObject`。
- **cpython312-pyc-compiler（可选）**：通过嵌入式 CPython 3.12 生成 `.pyc`（对应 GN 目标 `saauso_cpython312_compiler`）。

## 4. 代码规范（人类程序员和AI助手都必须严格遵守）

S.A.A.U.S.O 的代码规范主要基于 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)。

### 4.1. 代码文件规范
- **文件**:
  - C++ 源文件: `*.cc`
  - C++ 头文件: `*.h`
  - 文件名: `kebab-case` (例如 `py-code-object.cc`)。
- **头文件依赖**: 
  - 为降低编译期依赖复杂度，减少编译/链接错误，优先使用前置声明。
  - 仅在需要完整类型（例如按值成员、继承、`sizeof`/`alignof`、模板/内联实现）时才 `#include`。

### 4.2. 代码风格
- **命名空间**: 所有内部代码必须位于 `namespace saauso::internal` 中。
- **通用命名规则**
  - 名称应该是描述性的（Descriptive）。例如`int num_errors;`, `int num_dns_connections;`。
  - 严禁使用非通用缩写。允许使用业界通用缩写（如 `cpu`/`gc`/`vm`/`ir`/`abi`/`utf8`、`dns`、`url`）与仓库既有缩写。不要通过删除单词中的字母来缩写（例如严禁使用`cstmr_id`代替`customer_id`）。
- **类**: `PascalCase`。
- **变量**: `snake_case`，类成员变量以 `_` 结尾。
- **常量**: `kPascalCase`（例如`constexpr int kDaysInAWeek = 7;`, `const std::string kAndroid8_0 = "Android 8.0";`）
- **函数**: `PascalCase`（例如 `InitMetaArea()`、`IterateRoots()`、`RecordWrite()`）。
  - 例外：类属性的访问器与修改器，可以与变量命名风格对齐。Getter使用变量名本身（不要加`get_`前缀），Setter使用`set_ + 变量名`。例如：
    ```cpp
    class MyClass {
    public:
      int count() const { return count_; }        // Getter
      void set_count(int count) { count_ = count; } // Setter
    private:
      int count_;
    };
    ```

- **枚举值**: `kCamelCase`，例子：
  ```cpp
  enum UrlTableErrors {
    kOk = 0,
    kOutOfMemory,
    kMalformedInput,
  };
  ```
- **宏**: `MACRO_CASE`。在头文件中定义宏需要非常谨慎，应保证命名独特，以免污染全局命名空间。
- **类/结构体声明（头文件）**: 
  1. 访问控制段按 `public`→`protected`→`private`
  2. 数据成员集中放在末尾并尽量为 `private`（除非确有继承扩展需要）。
  3. 每段内部按以下顺序排列：
      - 类型与类型别名（例如typedef、using、enum、嵌套struct/class、友元类型）
      - 静态常量
      - 工厂函数
      - 构造函数与赋值运算符
      - 析构函数
      - 其他所有函数（静态与非静态成员函数，以及友元函数）
      - 其他所有数据成员（静态与非静态）

### 4.3. 注释风格
- **整体风格 (Generic Comment Style)**：
  - 统一使用`//`风格。
  - 除文件头部的版权声明外，正文代码统一采用**简体中文注释**。
  - 注释应像正式文档一样编写，注意英文术语大小写和中文标点符号。
  - 完整的句子通常比句子片段或词组更具可读性。
- **文件注释 (File Comments)**：
  - 版权声明：所有新文件必须包含如下的标准许可证头。
  ```cpp
  // Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
  // Use of this source code is governed by a GNU-style license that can be
  // found in the LICENSE file.
  ```
  - 如果一个头文件（`.h`）声明了多个面向用户的抽象概念（如一组相关的类或函数），应在文件开头进行整体描述。
      - **注意**：具体的类或函数文档应放在各自的声明处，不要堆砌在文件开头。
  - `.cc` 文件通常不需要文件级注释，除非它包含不属于任何头文件的独立实现细节。
- **类与结构体注释 (Class and Struct Comments)**
  - **必须写注释的情况**：所有非显而易见（non-obvious）的类或结构体都必须有注释。
  - **内容**：描述该类的用途（what it is for）以及如何使用（how it should be used）。
    - **可选**：如果确认该类或结构体属于长时间内不会发生变动的基建，可以提供一小段示例代码。
  - **示例**：
  ```cpp
  // 用于遍历GargantuanTable中的内容。
  // 示例:
  //    std::unique_ptr<GargantuanTableIterator> iter = table->NewIterator();
  //    for (iter->Seek("foo"); !iter->done(); iter->Next()) {
  //      process(iter->key(), iter->value());
  //    }
  class GargantuanTableIterator {
    ...
  };
  ```
- **声明处的函数注释 (Function Declaration Comments)**
  - **位置**：几乎每个函数的声明前都应有注释。
  - **内容**：描述函数做什么（what it does）以及如何使用（how to use it）。不要描述它是如何实现的（这是定义处注释的任务）。
  - **措辞**：使用祈使句语气，以动词开头（隐含主语"该函数"）。例如："打开文件"。
  - **必须包含的细节**：
    - 输入参数和输出结果。尤其是当参数含义不明显时（例如传入true/false开关）。
    - 指针/Handle参数是否允许为`null`。
    - 如有：需特别说明的其他内容（例如使用该函数对性能的影响等）。
- **定义处的函数注释 (Function Definition Comments)**
  - **内容**：描述函数是如何工作的（how it does its job）。
  - **适用场景**：解释复杂的实现逻辑、算法步骤、或者是为什么选择了这种实现方式而非其他方案。
  - **避免**：不要简单重复声明处的注释。
- **函数实现细节注释（Function Implementation Comments）**
  - **代码块**：在晦涩、复杂或巧妙的代码段前添加解释性注释。
  - **行尾注释**：如果是简短的说明，可以放在代码行尾，但要注意格式整洁。
- **TODO 注释**：使用全大写的TODO，例如`TODO: 当X模块完成后，移除此处逻辑`。


## 5. 开发工作流

### 5.1. 构建
根 `BUILD.gn` 当前主要提供：
- `saauso_core`：核心实现 `source_set`（虚拟机主体）。
- `saauso_cpython312_compiler`：嵌入式 CPython 3.12 编译器前端 `source_set`（可选）。
- `vm`：示例入口（见 `src/main.cc`）。
- `ut`：单元测试入口（转发到 `//test/unittests:ut`）。

项目本地包含了 `depot_tools`（Windows/Linux amd64），可直接调用其中的 `gn`/`ninja`。

Windows 上默认使用 Clang/LLD 工具链（见 `build/` 与 `build/toolchain/`），ASan 链接依赖 `llvm_lib_path`（默认 `D:\LLVM\lib\clang\21\lib\windows`）。如果你本地 LLVM 安装路径不同，请通过 `gn args` 或 `--args="llvm_lib_path=... is_asan=true"` 覆盖。

推荐优先使用仓库自带脚本（需要 Bash 环境，如 Git Bash/MSYS2/WSL）：
```bash
./build.sh release
./build.sh debug
./build.sh asan
./build.sh ut
```

在 Windows PowerShell 下可直接手动执行（注意目标名为 `vm`/`ut`）：
```powershell
# 生成构建文件
.\depot_tools\gn.exe gen out/debug --args="is_debug=true"

# 构建
.\depot_tools\ninja.exe -C out/debug vm

# 导出 compile_commands.json (供 clangd/IDE 使用)
.\depot_tools\gn.exe gen out/debug --args="is_debug=true" --export-compile-commands
```

### 5.2. 测试
单元测试位于 `test/unittests`。
```powershell
# 构建测试目标
.\depot_tools\gn.exe gen out/ut --args="is_asan=true"
.\depot_tools\ninja.exe -C out/ut ut

# 运行测试
.\out\ut\ut.exe
```

### 5.3. 单元测试架构（最新）
- **公共测试代码（当前布局）**：
  - `test/unittests/test-helpers.{h,cc}`：提供带生命周期的测试夹具基类（负责 `Saauso::Initialize/Dispose`、`Isolate` 创建与 Enter/Exit 等）与解释器输出捕获夹具。
  - `test/unittests/test-utils.{h,cc}`：提供无状态的小工具与断言谓词（例如 `IsPyStringEqual`、`AppendExpected`、`CompileScript312`、pyc 字节构造器等）。
- **测试目录分层（按模块分类）**：
  - `test/unittests/interpreter/`：解释器端到端用例（`interpreter-*-unittest.cc`）。
  - `test/unittests/objects/`：对象系统与容器/属性相关用例（`py-*-unittest.cc`、`attribute-unittest.cc`）。
  - `test/unittests/heap/`：GC/堆相关用例（如 `gc-unittest.cc`）。
  - `test/unittests/handles/`：句柄与并发相关用例（如 `handle-*-unittest.cc`、`global-handle-unittest.cc`）。
  - `test/unittests/code/`：pyc 解析/编译前端相关用例（如 `pyc-file-parser-unittest.cc`）。
  - `test/unittests/utils/`：纯工具/算法相关用例（如 `string-search-unittest.cc`）。
- **统一夹具基类**：
  - `VmTestBase`：适用于绝大多数“需要完整虚拟机环境”的单测。
  - `EmbeddedPython312VmTestBase`：在 `VmTestBase` 基础上额外负责 `FinalizeEmbeddedPython312Runtime()`，用于需要 CPython312 编译/解析前端的单测。
  - `IsolateOnlyTestBase`：仅创建/销毁 `Isolate`（不 Enter），用于线程隔离类测试。
- **解释器端到端测试**：
  - 统一使用 `BasicInterpreterTest` 夹具（print 注入 + 输出捕获）。
  - 用例按主题拆分到 `interpreter-*-unittest.cc`，避免单文件职责膨胀。
- **LSan 抑制**：
  - `__lsan_default_suppressions()` 统一放在 `test/unittests/lsan-suppressions.cc`，避免多处重复定义。
- **新增测试文件的接入点**：
  - 新增 `test/unittests/**/**.cc` 后，需要同步将其加入 `test/unittests/BUILD.gn` 的 `ut` 目标 `sources` 列表。
  - 根 `BUILD.gn` 仅保留 `group(\"ut\")` 作为入口（转发到 `//test/unittests:ut`），避免测试 sources 污染根构建文件。

## 6. 给 AI Agent 的关键实现细节
### 6.1. Handle / Tagged 使用规则（非常重要）
- **禁止在接口上传递 `PyObject*`**：`PyObject` 语义上可能承载 Smi（并非真实对象指针），传裸指针会导致 C++ UB；对外暴露与内部调用都应使用 `Tagged<PyObject>` 或 `Handle<PyObject>`。
- 该约束在源码注释中被视为“设计硬性前提”，相关说明集中在 `src/objects/py-object.h`。
- **栈上持有 GC-able 对象必须用 Handle**：只要对象可能在新生代中被复制移动，就必须用 `Handle<T>` 防止悬垂引用。
- **跨 HandleScope 返回要 Escape**：常见模式是在函数内创建 `EscapableHandleScope scope;`，然后 `return scope.Escape(result);`。
- **Tagged 等价于“带额外语义的裸指针”**：除永久区对象与短生命周期临时值外，不要把 `Tagged` 长时间放在栈/全局中；如果需要跨作用域/长期持有，请使用 `Global<T>`。

#### Global（长期句柄，类似 v8::Global）
`Global<T>` 用于长期持有 GC-able 的 Python 对象引用，它不受 `HandleScope` 生命周期影响，但自身遵循 C++ RAII：析构或 `Reset()` 时自动释放对对象的引用。

- 头文件：`src/handles/global-handles.h`
- 语义要点：
  - `Global<T>` 是 move-only（不可拷贝），避免双重释放。
  - `Global<T>` 会被纳入 GC roots，minor GC 后其内部 slot 会自动更新到新地址。
  - `Global<T>::Get()` 用于把全局句柄临时“降级”为栈上 `Handle<T>` 参与常规 API 调用；要求当前线程处于同一个 `Isolate::Current()`，且必须在某个 `HandleScope` 内调用。

用法示例：
```cpp
#include "src/handles/global-handles.h"
#include "src/handles/handles.h"

using namespace saauso::internal;

void Example() {
  HandleScope scope;
  Handle<PyString> s = PyString::NewInstance("hello");

  Global<PyString> g(s);

  {
    HandleScope inner;
    Handle<PyString> local = g.Get();
    (void)local;
  }

  g.Reset();
}
```

### 6.2. 分配与初始化
- **堆对象分配**：不要对 `PyObject` 派生对象使用 `new`；应使用 `Isolate::Current()->heap()->Allocate<T>(Heap::AllocationSpace::kNewSpace / kOldSpace / kMetaSpace)` 获取 `Tagged<T>`，并显式写入字段与 `PyObject::SetKlass(...)`。
- **分配不调用构造函数**：
  - `Heap::Allocate/AllocateRaw` 只返回原始内存（不保证分配得到的内存块已清零），不会执行 C++ 构造函数
  - 因此不要依赖构造函数为对象/表结构写入默认值。
  - 一般地，写入默认值操作在对应对象的工厂函数`XXX::NewInstance(...)`（比如PyString::NewInstance、PyDict::NewInstance）中手工完成。
- **永久区单例**：`None/True/False` 通过 `kMetaSpace` 分配并保存在 `Isolate`，通常不需要 `Handle` 保护。

### 6.3. GC 与遍历约定
- **对象大小**：GC 扫描依赖 `Klass::vtable_.instance_size(self)` 返回正确实例大小。
- **引用遍历**：每个可回收对象类型必须在 `Klass::vtable_.iterate(self, v)` 中准确访问其内部所有 `Tagged<PyObject>` 字段；否则会出现对象丢失或悬垂。
- **Forwarding**：新生代复制时，旧对象 `MarkWord` 会暂存 forwarding 地址（tag `0b10`），判断逻辑在 `MarkWord` 中。

### 6.4. 新增一个内建对象类型的最小步骤
- 在 `src/objects/` 新增 `py-xxx.{h,cc}` 与 `py-xxx-klass.{h,cc}`（文件名使用 `kebab-case`）。
- 如果对象在堆上有实体，加入 `PY_TYPE_IN_HEAP_LIST`（位于 `src/objects/py-object.h`），以便自动生成 `IsPyXxx(...)` 等检查器。
- 在对应 `Klass::PreInitialize()` 填充 vtable（至少需要 `instance_size` 与 `iterate`），并在 `Finalize()` 做必要清理。
- 将该类型加入 `src/runtime/isolate-klass-list.h` 的 `ISOLATE_KLASS_LIST`，保证：
  - `Isolate` 拥有对应的 `*_klass()` accessor 与字段；
  - `Isolate::InitMetaArea()` 会自动执行该 Klass 的 `InitializeVTable/PreInitialize/Initialize`；
  - `Isolate::TearDown()` 会自动执行该 Klass 的 `Finalize`。
- 将新文件加入根 [BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/BUILD.gn) 的 `saauso_core.sources` 列表，保证目标可链接。
- （可选）在 `src/interpreter/interpreter.cc` 注册该类型的 `type_object()`，并加入 `builtins` 字典。
  - 这步不是必须选项，只有当我们希望将这种类型泄露到 Python 语言中，允许用户代码中使用 `type(...)` 或 `isinstance(..., ...)` 等操作时才需要。
  - 例如在Python中，NoneType这种类型就没有被泄漏到Python语言环境，因此它不需要注册到`builtins`字典中。

### 6.5. `src/utils/` 的依赖边界（判定口径）
- **目标**：将 `src/utils/` 维持为“纯工具层”，避免引入虚拟机上层耦合，降低编译期依赖与循环依赖风险。
- **允许依赖**：
  - `src/build/` 中的编译控制宏（如 `BUILDFLAG`、`IS_WIN`、`IS_LINUX`等）。
  - `src/utils/` 目录内的其它工具模块。
  - `third_party/` 中的第三方库。 
  - C++ 标准库。
- **禁止依赖**：
  - `src/runtime/`、`src/heap/`、`src/handles/`、`src/objects/`、`src/interpreter/` 及其暴露的能力。
  - 直接或间接访问 `Isolate::Current()`、`HandleScope`、`Heap::Allocate` 等会把 `utils` 变成“VM 上层的一部分”的接口。
- **例外处理**：若确需跨模块共享基础类型或小型工具，优先下沉到 `include/` 或在 `src/utils/` 内提供与 VM 无关的抽象接口，再由上层实现适配。
