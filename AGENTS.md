# S.A.A.U.S.O 项目 - AI 助手指南

本文档概述了 S.A.A.U.S.O 项目的架构决策、代码标准和开发工作流。AI 助手在生成代码时应参考本文档，以确保与现有代码库保持一致。

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
- **语言**: C++ (Modern C++, likely C++20)。
- **构建系统**: GN (Generate Ninja) + Ninja。
- **测试框架**: Google Test。
- **辅助调试工具**: LLDB、AddressSanitizer、UBSanitizer

## 2. 仓库结构速览
- `include/`：对外/跨模块共享的基础定义（例如 `Address`、Smi/对齐等）。
- `src/code/`：`.pyc` 等二进制代码文件解析（`BinaryFileReader` / `PycFileParser`）。
- `src/objects/`：对象系统（`PyObject`、`Klass`、各内建对象与其 `*-klass`）。
- `src/handles/`：句柄系统（`Handle`/`HandleScope`/`HandleScopeImplementer`）与 `Tagged<T>`。
- `src/heap/`：堆与空间（`NewSpace`/`OldSpace`/`MetaSpace`）以及新生代 GC（Scavenge）。
- `src/runtime/`：全局运行时容器（`Universe`）与隔离性探索（`isolate.h` 仍处于占位状态）。
- `src/utils/`：通用工具（对齐/内存/小型容器等）。
- `test/unittests/`：基于 GTest 的单元测试。
- `BUILD.gn`：根目标定义（当前主要目标：`vm`、`ut`）。
- `build/`：编译配置与工具链（Clang/LLD，支持 `is_debug`/`is_asan`）。
- `testing/gtest/`：项目内置的 GTest GN 封装目标（供 `ut` 依赖）。
- `third_party/`：第三方代码（例如 `googletest` 上游源码、`rapidhash`）。

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
- **Klass 也是 GC Root**:
  - `Klass::Iterate(ObjectVisitor*)` 负责把 `Klass` 自身持有的引用暴露给 GC；新增字段时必须同步更新。
- **MRO/C3 与属性查找**:
  - `Klass::supers_` 语义上保存父类 `PyTypeObject` 列表；`OrderSupers()` 用 C3 线性化生成 `mro_`，并通常同时写入 `klass_properties_["mro"]`。
  - 默认 getattr 流程：先沿 MRO 查 `__getattr__`，再查实例 dict（仅 heap object），最后沿 MRO 查类 dict；命中函数时包装成 `MethodObject(func, self)`。
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
  - **NewSpace**: 复制算法（Eden + Survivor，Flip）。
  - **OldSpace**: 已划分空间，但回收逻辑仍在 TODO（目前可能出现老生代 OOM 即退出）。
  - **MetaSpace**: 永久区（例如 `None/True/False` 等全局单例与 `Klass` 相关数据）。
- **根集合 (Roots)**:
  - `Universe::klass_list_`：遍历各 `Klass` 内部引用。
  - `HandleScopeImplementer`：遍历所有 handle blocks 内引用。
  - Python 运行时根（解释器/栈帧/全局变量等）仍在 TODO（见 `Heap::IterateRoots`）。
- **句柄系统 (`src/handles`)**:
  - `Handle<T>` 是“会被 GC 移动/回收的对象”的栈上安全持有方式；`Tagged<T>` 更接近裸指针语义。
  - `HandleScope` 管理 handle 生命周期；跨 scope 返回时使用 `EscapeFrom`。
  - 永久区对象（例如 `Universe::py_none_object_` / `py_true_object_` / `py_false_object_`）分配在 `kMetaSpace`，不会被回收移动，通常可以直接用 `Tagged<T>` 返回/保存。
- **TODO**: 如果虚拟机的基础功能开发完毕后仍有多余时间，再进行分代式GC的开发。

### 3.3. 运行时 (`src/runtime`)
- **Universe**: 全局静态容器，包含堆 (Heap)、句柄作用域实现 (HandleScopeImplementer)、解释器 (Interpreter)、字符串表 (StringTable) 与全局单例（如 `None`, `True`, `False`）。
- **Interpreter**: 当前字节码执行与内建函数注册入口（内建函数实现在 `native-functions.*`）。
- **StringTable**: 常用字符串常量池（通常在 `kMetaSpace` 分配字符串对象，避免被 GC 移动）。
- **隔离性**: `isolate.h` 目前仅为占位雏形；主路径仍依赖 `Universe` 静态全局，重构为多 Isolate 时需系统性清理静态状态。

### 3.4. 代码加载 (`src/code`)
- **PycFileParser**: 负责解析 `.pyc` 文件并构建 `PyCodeObject`（依赖 `BinaryFileReader`）。

## 4. 代码规范

### 4.1. 风格
- **标准**: 遵循 Google C++ Style Guide。
- **命名空间**: 所有内部代码必须位于 `namespace saauso::internal` 中。
- **文件**:
  - C++ 源文件: `*.cc`
  - C++ 头文件: `*.h`
  - 文件名: `kebab-case` (例如 `py-code-object.cc`)。
- **类**: `PascalCase`。
- **变量**: `snake_case`，类成员变量以 `_` 结尾。

### 4.2. 版权头
所有新文件必须包含标准的许可证头：
```cpp
// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.
```

## 5. 开发工作流

### 5.1. 构建
根 `BUILD.gn` 当前主要提供两个可执行目标：
- `vm`：示例入口（见 `src/main.cc`）。
- `ut`：单元测试入口（聚合 `test/unittests/*.cc`）。

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

## 6. 给 AI Agent 的关键实现细节
### 6.1. Handle / Tagged 使用规则（非常重要）
- **禁止在接口上传递 `PyObject*`**：`PyObject` 语义上可能承载 Smi（并非真实对象指针），传裸指针会导致 C++ UB；对外暴露与内部调用都应使用 `Tagged<PyObject>` 或 `Handle<PyObject>`。
- **栈上持有 GC-able 对象必须用 Handle**：只要对象可能在新生代中被复制移动，就必须用 `Handle<T>` 防止悬垂引用。
- **跨 HandleScope 返回要 Escape**：常见模式是在函数内创建 `HandleScope scope;`，然后 `return result.EscapeFrom(&scope);`。
- **Tagged 等价于“带额外语义的裸指针”**：除永久区对象与短生命周期临时值外，不要把 `Tagged` 长时间放在栈/全局中。

### 6.2. 分配与初始化
- **堆对象分配**：不要对 `PyObject` 派生对象使用 `new`；应使用 `Universe::heap_->Allocate<T>(Heap::AllocationSpace::kNewSpace / kOldSpace / kMetaSpace)` 获取 `Tagged<T>`，并显式写入字段与 `PyObject::SetKlass(...)`。
- **永久区单例**：`None/True/False` 通过 `kMetaSpace` 分配并保存在 `Universe`，通常不需要 `Handle` 保护。

### 6.3. GC 与遍历约定
- **对象大小**：GC 扫描依赖 `Klass::vtable_.instance_size(self)` 返回正确实例大小。
- **引用遍历**：每个可回收对象类型必须在 `Klass::vtable_.iterate(self, v)` 中准确访问其内部所有 `Tagged<PyObject>` 字段；否则会出现对象丢失或悬垂。
- **Forwarding**：新生代复制时，旧对象 `MarkWord` 会暂存 forwarding 地址（tag `0b10`），判断逻辑在 `MarkWord` 中。

### 6.4. 新增一个内建对象类型的最小步骤
- 在 `src/objects/` 新增 `py-xxx.{h,cc}` 与 `py-xxx-klass.{h,cc}`（文件名使用 `kebab-case`）。
- 如果对象在堆上有实体，加入 `PY_TYPE_IN_HEAP_LIST`（位于 `src/objects/py-object.h`），以便自动生成 `IsPyXxx(...)` 等检查器，并确保 `Universe::Genesis()` 初始化 `Klass`。
- 在对应 `Klass::Initialize()` 填充 vtable（至少需要 `instance_size` 与 `iterate`），并在 `Finalize()` 做必要清理。
- 若新增的 `Klass` 不在 `PY_TYPE_LIST` 的自动初始化流程中（例如 `NativeFunctionKlass` / `MethodObjectKlass` 这类特化类型），需要同步更新 `Universe::InitMetaArea()` 与 `Universe::Destroy()` 的手动初始化/反初始化路径。
- 将新文件加入根 [BUILD.gn](file:///c:/Users/Administrator/Desktop/S.A.A.U.S.O/BUILD.gn) 的 `saauso_sources` 列表，保证目标可链接。
