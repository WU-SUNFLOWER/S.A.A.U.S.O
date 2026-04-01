# S.A.A.U.S.O VM 工程约束与编程范式约定

本文档收口 S.A.A.U.S.O 虚拟机内部实现的关键工程约束（UB/GC 安全/对象分配与遍历/依赖边界等）与编程范式约定，用于指导人类程序员和 AI 助手保确新增代码的正确性、可维护性，以及与现有代码的一致性。

## 1. Handle / Tagged 使用规则（非常重要）

- **禁止在接口上传递** **`PyObject*`**：`PyObject` 语义上可能承载 Smi（并非真实对象指针），传裸指针会导致 C++ UB；对外暴露与内部调用都应使用 `Tagged<PyObject>` 或 `Handle<PyObject>`。
  - 该约束在源码注释中被视为“设计硬性前提”，相关说明集中在 `src/objects/py-object.h`。
- **栈上持有 GC-able 对象必须用 Handle**：只要对象可能在新生代中被复制移动，就必须用 `Handle<T>` 防止悬垂引用。
- **跨 HandleScope 返回要 Escape**：常见模式是在函数内创建 `EscapableHandleScope scope;`，然后 `return scope.Escape(result);`。
- **Handle 系统依赖具体的 Isolate 实例**：在创建 `HandleScope`/ `Handle`，或使用 `Global::Get()` 时，必须提供有效的 Isolate 实例指针，并确保 `Handle` 所指对象的确位于该 Isolate 的虚拟机堆上。
- **Tagged 等价于“带额外语义的裸指针”**：除永久区对象与短生命周期临时值外，不要把 `Tagged` 长时间放在栈/全局中；如果需要跨作用域/长期持有，请使用 `Global<T>`。

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
  - `src/build/` 中的编译控制宏（如 `BUILDFLAG`、`IS_WIN`、`IS_LINUX` 等）。
  - `src/utils/` 目录内的其它工具模块。
  - `third_party/` 中的第三方库。
  - C++ 标准库。
- **禁止依赖**
  - `src/runtime/`、`src/heap/`、`src/handles/`、`src/objects/`、`src/interpreter/` 及其暴露的能力。
  - 直接或间接访问 `Isolate::Current()`、`HandleScope`、`Heap::Allocate` 等会把 `utils` 变成“VM 上层的一部分”的接口。
- **例外处理**：若确需跨模块共享基础类型或小型工具，优先下沉到 `include/` 或在 `src/utils/` 内提供与 VM 无关的抽象接口，再由上层实现适配。

