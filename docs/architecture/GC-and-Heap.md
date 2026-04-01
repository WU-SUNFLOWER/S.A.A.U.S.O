# 内存管理：GC 与堆（GC and Heap）

本文档收口 S.A.A.U.S.O 的堆空间模型、根集合与 GC（当前以新生代 Scavenge 为主）的设计与现状说明。

## 1. 堆与空间模型（`src/heap`）

- **垃圾回收 (GC)**：当前实现以新生代 Scavenge 为主。
  - **NewSpace**：复制算法（Eden + Survivor，Flip）。注意：当前 `NewSpace::Contains()` 仅判断 Eden；涉及空间判定时优先使用 `Heap::InNewSpaceEden()` / `Heap::InNewSpaceSurvivor()` 或直接对 `eden_space()/survivor_space()` 做 Contains。
  - **OldSpace**：地址段已预留，但 `AllocateRaw/Contains` 尚未实现（当前会触发断言）；MVP 主路径应避免 OldSpace 分配，老生代回收仍在 TODO。
  - **MetaSpace**：永久区（例如 `None/True/False` 等全局单例与 `Klass` 相关数据）。

## 2. 根集合（Roots）

- `Isolate::klass_list_`：遍历各 `Klass` 内部引用（通常由各 `Klass::PreInitialize()` 注册）。
- `HandleScopeImplementer`：遍历所有 handle blocks 内引用。
- `Interpreter`：解释器内部持有的引用也会被遍历（见 `Heap::IterateRoots`）。
- `ExceptionState`：pending exception 属于 GC root（见 `Heap::IterateRoots`）。
- `ModuleManager`：遍历模块系统中持有的引用（`sys.modules/sys.path`），保证 import 引入的模块与路径在 minor GC 下可达。
- `StringTable` 当前把常用字符串驻留在 MetaSpace，`Heap::IterateRoots` 暂未开放其遍历入口。
- Python 运行时根（更完整的栈帧/全局变量等）仍在 TODO（见 `Heap::IterateRoots`）。

## 3. Remembered set / Write barrier（现状）

- `Heap::RecordWrite()` 与 `remembered_set_` 已实现雏形，用于记录 “Old -> New” 跨代写入。
- 目前通过 `WRITE_BARRIER` 宏强制为空、且 `IterateRoots()` 内的 `IterateRememberedSet()` 调用被禁用，MVP 不依赖该机制。

## 4. 句柄系统与 GC（与 `src/handles` 的关系）

- `Handle<T>` 是“会被 GC 移动/回收的对象”的栈上安全持有方式；`Tagged<T>` 更接近裸指针语义。
- `HandleScope` 管理 handle 生命周期；跨 scope 返回时应当使用语义更加明确的 `EscapableHandleScope`。
- 永久区对象（例如 `Isolate::py_none_object()` / `py_true_object()` / `py_false_object()`）分配在 `kMetaSpace`，不会被回收移动，通常可以直接用 `Tagged<T>` 返回/保存。

## 5. TODO

如果虚拟机的基础功能开发完毕后仍有多余时间，再进行分代式 GC 的开发。当前的目标是实现一个仅含有新生代 scavenge GC 的 MVP 版本。

