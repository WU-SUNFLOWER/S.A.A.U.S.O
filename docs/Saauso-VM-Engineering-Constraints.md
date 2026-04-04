# S.A.A.U.S.O VM 工程约束与编程范式约定

本文档收口 S.A.A.U.S.O 虚拟机内部实现的关键工程约束（UB/GC 安全/对象分配与遍历/依赖边界等）与编程范式约定，用于指导人类程序员和 AI 助手保确新增代码的正确性、可维护性，以及与现有代码的一致性。

## 1. Handle / Tagged 使用规则（非常重要）

- **禁止在接口上传递** **`PyObject*`**：`PyObject` 语义上可能承载 Smi（并非真实对象指针），传裸指针会导致 C++ UB；对外暴露与内部调用都应使用 `Tagged<PyObject>` 或 `Handle<PyObject>`。
  - 该约束在源码注释中被视为“设计硬性前提”，相关说明集中在 `src/objects/py-object.h`。
- **栈上持有 GC-able 对象必须用 Handle**：只要对象可能在新生代中被复制移动，就必须用 `Handle<T>` 防止悬垂引用。
- **跨 HandleScope 返回要 Escape**：常见模式是在函数内创建 `EscapableHandleScope scope;`，然后 `return scope.Escape(result);`。
- **Handle 系统依赖具体的 Isolate 实例**：在创建 `HandleScope`/ `Handle`，或使用 `Global::Get()` 时，必须提供有效的 Isolate 实例指针，并确保 `Handle` 所指对象的确位于该 Isolate 的虚拟机堆上。
- **Tagged 等价于“带额外语义的裸指针”**：除永久区对象与短生命周期临时值外，不要把 `Tagged` 长时间放在栈/全局中；如果需要跨作用域/长期持有，请使用 `Global<T>`。
- **只要路径可能触发 Python 代码、分配或异常传播，就不得依赖裸 `this`、裸 `Tagged`、裸 `buffer` 或裸尾随元素指针跨该路径存活**：这条原则适用于基础容器、字符串、对象属性访问与所有 runtime helper。

### 1.1 Global（长期句柄，类似 v8::Global）

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

## 2. 分配与初始化

- **堆对象分配**：不要对 `PyObject` 派生对象使用 `new`；业务代码应优先走 `isolate->factory()->NewXxx(...)`。仅在 `Factory` 内部才直接触达 `Heap::Allocate*`。
- **获取内建 Python 常量的 Handle 包装**：一般不建议手写"获取内建常量 tagged -> 包装成Handle"，`Factory` 中已经提供可直接使用的便捷函数：
  - 获取 True：`isolate->factory()->py_true_object()`
  - 获取 False：`isolate->factory()->py_false_object()`
  - 获取 None：`isolate->factory()->py_none_object()`
  - C++ bool 转 Python 对象：`isolate->factory()->ToPyBoolean(condition)`
- **分配不调用构造函数**
  - `Heap::Allocate/AllocateRaw` 只返回原始内存（不保证分配得到的内存块已清零），不会执行 C++ 构造函数。
  - 因此不要依赖构造函数为对象/表结构写入默认值。
  - 一般地，写入默认值操作在 `Factory::NewXxx(...)` 或对应对象 `XXX::New(...)` 路径中手工完成。
- **初始化阶段分配约束**：`Factory` 在“先写哨兵字段、后续再做可能分配动作”的关键路径使用 `DisallowHeapAllocation`，避免对象半初始化状态下触发二次分配。
- **永久区单例**：`None/True/False` 通过 `kMetaSpace` 分配并保存在 `Isolate`，通常不需要 `Handle` 保护。

## 3. GC 与遍历约定

- **对象大小**：GC 扫描依赖 `Klass::vtable_.instance_size(self)` 返回正确实例大小。
- **引用遍历**：每个可回收对象类型必须在 `Klass::vtable_.iterate(self, v)` 中准确访问其内部所有 `Tagged<PyObject>` 字段；否则会出现对象丢失或悬垂。
- **Forwarding**：新生代复制时，旧对象 `MarkWord` 会暂存 forwarding 地址（tag `0b10`），判断逻辑在 `MarkWord` 中。

## 4. 新增一个内建对象类型的最小步骤

- 在 `src/objects/` 新增 `py-xxx.{h,cc}` 与 `py-xxx-klass.{h,cc}`（文件名使用 `kebab-case`）。
- 如果对象在堆上有实体，加入 `PY_TYPE_IN_HEAP_LIST`（位于 `src/objects/object-type-list.h`），以便自动生成 `IsPyXxx(...)` 等检查器。
- `PY_TYPE_*` 与 `ISOLATE_KLASS_LIST` 是“同域不同职责”：前者服务 checker/类型清单复用，后者服务 Isolate klass 槽位与初始化流程。
- 在对应 `Klass::PreInitialize()` 填充 vtable（至少需要 `instance_size` 与 `iterate`），并在 `Finalize()` 做必要清理。
- 将该类型加入 `src/execution/isolate-klass-list.h` 的 `ISOLATE_KLASS_LIST`，保证：
  - `Isolate` 拥有对应的 `*_klass()` accessor 与字段。
  - `Isolate::InitMetaArea()` 会自动执行该 Klass 的 `InitializeVTable/PreInitialize/Initialize`。
  - `Isolate::TearDown()` 会自动执行该 Klass 的 `Finalize`。
- 将新文件加入根 [BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/BUILD.gn) 的 `saauso_core.sources` 列表，保证目标可链接。
- （可选）在 [builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc) 的 `InstallBuiltinTypes()` 中注册该类型的 `type_object()`，并加入 `builtins` 字典。
  - 这步不是必须选项，只有当希望将该类型泄露到 Python 语言中，允许用户代码中使用 `type(...)` 或 `isinstance(..., ...)` 等操作时才需要。
  - 例如在 Python 中，NoneType 这种类型就没有被泄漏到 Python 语言环境，因此它不需要注册到 `builtins` 字典中。

## 5. `src/utils/` 的依赖边界（判定口径）

- **目标**：将 `src/utils/` 维持为“纯工具层”，避免引入虚拟机上层耦合，降低编译期依赖与循环依赖风险。
- **允许依赖**
  - `build/` 中的编译控制宏（如 `BUILDFLAG`、`IS_WIN`、`IS_LINUX` 等）。
  - `src/utils/` 目录内的其它工具模块。
  - `third_party/` 中的第三方库。
  - C++ 标准库。
- **禁止依赖**
  - `src/runtime/`、`src/heap/`、`src/handles/`、`src/objects/`、`src/interpreter/` 及其暴露的能力。
  - 直接或间接访问 `Isolate::Current()`、`HandleScope`、`Heap::Allocate` 等会把 `utils` 变成“VM 上层的一部分”的接口。
- **例外处理**：若确需跨模块共享基础类型或小型工具，优先下沉到 `include/` 或在 `src/utils/` 内提供与 VM 无关的抽象接口，再由上层实现适配。

## 6. 架构边界（必须遵守）

- **禁止**：`src/runtime` 与 `src/builtins` 直接 include 或调用 `src/interpreter`（包括 `#include "src/interpreter/interpreter.h"`、`Isolate::Current()->interpreter()->...` 等）。
- **允许**：`src/runtime` 与 `src/builtins` 通过 execution/runtime 提供的门面 API 完成调用与抛错：
  - 执行/调用入口：使用 `src/execution/execution.h` 的 `Execution::*`
  - 抛错与错误格式化：使用 `src/runtime/runtime-exceptions.h` 的 `Runtime_Throw*` / `Runtime_FormatPendingExceptionForStderr`
  - error indicator：由 `Isolate::exception_state()` 统一持有与传播
- **理由**：
  - 让依赖方向保持单向：`runtime/builtins -> execution -> interpreter`，避免循环依赖与“runtime 变半个解释器”。
  - 降低后续演进成本：未来引入其他执行器（AOT/JIT）时，只需要调整 execution 门面的实现，而不是全仓库逐点替换调用。
  - 控制职责边界：Isolate 负责状态（如 ExceptionState 与 builtins），Execution 负责门面（Call/CurrentFrame\*），interpreter 负责算法（dispatch/unwind）。

## 7. 错误传播契约（必须遵守）

### 7.1 Error Conventions（必须遵守）

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

### 7.2 dict API Conventions（必须遵守）

- **优先使用 runtime 语义入口**（当你需要 Python 语言层语义时）：
  - `Runtime_DictGetItem/Runtime_DictSetItem/Runtime_DictDelItem`：对应 `d[key]`/写入/删除的 KeyError 语义，并保证 `__hash__/__eq__` 异常原样传播。
- **允许直接使用 PyDict 原语的场景**（当你实现的是解释器更高层语义时）：
  - 例如 `LOAD_NAME/LOAD_GLOBAL` 的 locals→globals→builtins 链式查找（miss 继续，最终 `NameError`），该语义不等价于 `d[key]`。
  - 这类场景必须使用 fallible API：`Get/GetTagged/Put/Remove/ContainsKey`，并在失败时立即传播 pending exception。
- **内部初始化路径的约束**：
  - 即便是 key 可证明为 interned `PyString`（例如 `ST(...)` / 字节码 names 表），也必须使用 `Put/Get/GetTagged/...`。
  - 若调用点没有异常传播通道，可显式忽略返回值，但不得恢复旧 API。

## 8. 对象系统约束（必须遵守）

本节收口对象系统（`src/objects`）中与正确性强相关的约束：vtable slot 契约、cast like/exact 语义、对象布局与 Tagged/Smi 编码。

### 8.1 VirtualTable（slot 表）契约

- `Klass::vtable_` 保存各类操作的函数指针，新增/重写 slot 时：要么显式指向默认实现，要么确保所有调用点在 `nullptr` 时可处理（否则会空函数指针调用崩溃）。
- `getattr` 采用四参形式：`getattr(self, name, is_try, out)`，返回 `Maybe<bool>`（三态）：
  - `Nothing`：查询过程中发生异常（已设置 pending exception）
  - `true`：命中属性，`out` 写入属性值（非 null）
  - `false`：未命中属性，`out` 为 null；若 `is_try=false`，实现必须设置 `AttributeError` 并返回 `Nothing`
- 与 GC 遍历相关的硬约束（避免悬垂/对象丢失）请同时遵守本文件的 `## 3. GC 与遍历约定`。

### 8.2 `cast` 语义与 checker 边界

- `Handle<T>::cast` 依赖 `T::cast(...)` 做断言；若某类型未提供该断言函数，需要在 T 类型的 .h 及 .cc 文件中分别补充 `Tagged<T> T::cast(Tagged<PyObject> object)` 方法的定义与实现。
- 对 `list/str/tuple/dict` 这类“已建立 native layout 继承链”的内建容器，`T::cast` 的断言语义采用 like（布局兼容）而不是 exact（klass 全等）。
  - 判定 API 统一使用 `IsPyXxx`（like）与 `IsPyXxxExact`（exact）。
  - `IsPyString/IsPyList/IsPyDict/IsPyTuple` 默认是 like；若某调用点确实需要 exact 类型语义，必须显式使用 `IsPyStringExact/IsPyListExact/IsPyDictExact/IsPyTupleExact`。
- checker 模块边界：
  - 声明/实现位于 `src/objects/object-checkers.h/.cc`
  - 类型基建 list 位于 `src/objects/object-type-list.h`
  - `py-object.h` 通过 include 暴露 checker API，`py-object.cc` 不再承载 checker 定义

### 8.3 对象布局与 Tagged/Smi 编码

- `MarkWord` 必须位于对象内存布局起始位置；GC 与类型信息读取依赖它。
  - `MarkWord` 语义：要么保存 `Klass` 指针，要么在 GC 期间保存 forwarding 地址（tag 为 `0b10`）。
- `Tagged<T>` 统一表达“可能是堆对象，也可能是 Smi”的引用语义。
  - Smi：`Address` 末位为 `1`，值编码为 `SmiToAddress(v) = (v << 1) | 1`（见 `include/saauso-internal.h`）。
  - 堆对象：依赖对齐保证低位为 `0`（当前对齐为 8 字节，`kObjectAlignmentBits = 3`），不使用额外 tag 位。
  - `Tagged<T>::Cast` 的语义相当于 `reinterpret_cast`，只能在“你确信类型正确”时使用；类型断言通常由 `T::Cast(...)` 或 `IsPyXxx(...)` 承担。
