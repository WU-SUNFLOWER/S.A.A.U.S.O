# S.A.A.U.S.O —— 一个轻量级 Python 虚拟机

![Logo](logo/logo-with-title-light.png#gh-light-mode-only)

![Logo](logo/logo-with-title-dark.png#gh-dark-mode-only)

S.A.A.U.S.O 是一个轻量级 Python 虚拟机（后端），其实现了 Python 编程语言的一个核心功能子集。

它既可以作为独立运行的 Python 脚本解释执行系统，也可以作为脚本引擎嵌入 C++ 宿主程序。

## 1. 这个项目是什么

### 1.1 背景

Python 语言的执行依赖于Python虚拟机（PVM），主要包括前端和后端两大部分。
- 前端中由编译器（Compiler）将 Python 源代码（Source Code）编译成中间字节码（Bytecode）。
- 后端中由解释器（Interpreter）负责解释执行字节码。此外，后端还负责提供垃圾回收器（Garbage Collector）和运行时（Runtime）等 Python 语言运行所需的配套子系统。

目前，官方标准的 PVM 实现为 CPython，采用 C 语言开发。

### 1.2 本项目的定位

本项目是一个基于 C++20 实现一个可裁剪、易理解、易定制的轻量级 PVM 后端，兼容标准 CPython 3.12 的字节码规范。

**本项目的目的不在于复刻完整的 Python 语言，或者代替成熟的工业级 CPython 虚拟机；而在于完成一个具备清晰架构、适合教学展示、裁剪定制或二次开发的轻量级 PVM 后端，支持一个 Python 语言的核心功能子集。**

## 2. 为什么这个项目有价值

虽然由官方推出的 CPython 工业级虚拟机历经 30 余年的迭代，已经高度成熟；但本项目在教育教学、工程应用和人才培养方面，仍具有一定实际价值：
- **教育教学价值**: 
  - 相较于 CPython 数十万行代码的体量，本项目的核心代码只有 3 万多行。
  - 因此本项目更适合作为《编译原理》等系统类课程的教学用具，在课堂上进行演示或讲解。
- **工程与实际应用价值**: 
  - 本项目可作为办公软件、游戏引擎等宿主程序内的轻量级脚本引擎，而不是只能作为独立命令行解释器存在。
  - 本项目采用现代 C++ 语言进行开发，且实现了清晰的分层架构，十分便于开发人员对虚拟机进行裁剪、修改定制或二次开发。
- **人才培养价值**: 
  - 本项目不同于常见的 Web 前后端系统本科毕设，涉及语言运行时、对象系统、内存管理、异常机制、模块加载等底层核心问题，要求综合调动多门本科计算机专业核心课程的知识，可以充分锻炼选题学生的复杂系统设计与工程落地实现的完整能力。


## 5. 本项目支持的 Python 能力

### 5.1 概述

本项目支持的 Python 语言核心功能子集包括：
- 控制流：`if/while/break/continue`
- 内建类型：`list/dict/tuple/str/bool/None`
- 函数机制：位置参数、关键字参数、默认值、`*args/**kwargs`、闭包变量
- 类型系统：自定义类、类的继承、方法绑定、魔法方法
- 导入系统：模块、包、相对导入、内建模块
- 异常系统：抛出异常、异常传播、异常捕获、`finally`

### 5.2 典型例子

以下几个例子取自**本项目的单元测试集，且均已进行过回归验证**，用于说明本项目对 Python 语言核心能力的支持情况。

#### 例 1：基本的控制流

```python
def F(n):
    if n == 0:
        return 0
    elif n == 1:
        return 1
    else:
        return F(n - 1) + F(n - 2)

i = 0
while True:
  print(F(i))
  i += 1
  if i > 5:
    break
```

**这个例子用于说明**:  本项目已支持循环、判断、递归等基础的控制流。  

**出处**: [test/unittests/interpreter/interpreter-functions-unittest.cc](test/unittests/interpreter/interpreter-functions-unittest.cc) 中的 `FibRecursion`

#### 例 2：内建类型与函数的位置参数、关键字参数

```python
def send_email(sender, *recipients, **settings):
    print("Sender: " + sender)
    print("Recipients: " + ' '.join(recipients))

    # 提取配置，并提供默认值
    channel = settings.get('channel', 'email')
    priority = settings.get('priority', 'normal')

    print("Channel: " + channel + ", Priority: " + priority)

# 调用函数
send_email("Wang", "Mr.Song", "Mr.Liu", "Mr.Wu", channel="WeChat", priority="high")
```

**这个例子用于说明**：本项目已支持 Python 中的常用内建类型（`str`、`list`、`dict`），以及函数调用中的关键字参数和位置参数混合绑定。 

出处：[test/unittests/interpreter/interpreter-functions-unittest.cc](test/unittests/interpreter/interpreter-functions-unittest.cc) 中的 `ExtendPosAndKwArgsWithBusinessScenario`

#### 例 3：类的继承、方法绑定与魔法方法

```python
class Base(object):
  def __call__(self):
    print("Base is called")

class Derive(Base):
  def __init__(self, v):
    self.v = v

  def show(self):
    print(self.v)

  def __call__(self):
    print("Derive is called")

a = Derive(7)
f = a.show
f()  # output: 7
a()  # output: "Derive is called"
```

**这个例子用于说明**：本项目已支持自定义类继承、类的方法绑定，以及魔法方法。

**出处**: [test/unittests/interpreter/interpreter-custom-class-core-unittest.cc](test/unittests/interpreter/interpreter-custom-class-core-unittest.cc) 中的 `MagicMethodCanBeOverrided`

#### 例 4：异常的抛出与捕获

```python
try:
  x = len()
except TypeError:
  print("except")
finally:
  print("finally")
print("after")
```

**这个例子用于说明**：本项目已支持 Python 程序抛出异常，并且在程序执行流中对异常进行基本的捕获与处理。

**出处**: [test/unittests/interpreter/interpreter-exceptions-unittest.cc](test/unittests/interpreter/interpreter-exceptions-unittest.cc) 中的 `TryExceptFinallyWithExceptionEmitByVm`


## 3. 如何把虚拟机嵌入到你的应用程序

S.A.A.U.S.O 提供一套面向宿主程序的 Embedder API。  
宿主程序可以完成以下工作：

- 创建并管理 `Isolate/Context`
- 编译与执行脚本
- 在 C++ 与脚本之间交换值与对象
- 注入宿主回调给脚本调用
- 用 `TryCatch` 观察脚本执行异常

一个最小接入流程如下：

1. `Saauso::Initialize()`
2. `Isolate::New()`
3. `Isolate::Scope + HandleScope + Context::New()`
4. `Script::Compile()`
5. `Run(context)`
6. `TryCatch` 检查异常
7. `Dispose()`

最小示例：

```cpp
#include "saauso-embedder.h"
#include "saauso.h"

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) return 1;

  {
    saauso::Isolate::Scope isolate_scope(isolate);
    saauso::HandleScope scope(isolate);
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    if (context.IsEmpty()) return 1;

    saauso::TryCatch try_catch(isolate);
    auto maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "x = 1 + 2\n"));
    if (maybe_script.IsEmpty() || try_catch.HasCaught()) return 1;

    if (maybe_script.ToLocalChecked()->Run(context).IsEmpty() ||
        try_catch.HasCaught()) {
      return 1;
    }
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
```

如果你需要更完整的接入文档，请阅读：

- [Embedder API 接入指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Embedder-API-User-Guide.md)
- [Embedder API 架构设计](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Embedder-API-Architecture-Design.md)

## 6. 本项目的整体架构

本项目借鉴了现代工业级 VM 系统（如 V8、HotSpot）的分层架构设计。

目的是让对象表示、运行时语义、执行引擎与对外接口边界清晰、职责单一，便于进行长期拓展和维护。

![Architecture](./logo/architecture-light.png#gh-light-mode-only)
![Architecture](./logo/architecture-dark.png#gh-dark-mode-only)

从底层到高层，本项目包括：
- **`Utils`**：纯工具层，不依赖 VM 上层语义。
- **`Heap`**：自动内存管理系统，包括虚拟机堆、垃圾回收子系统、Handle 子系统等。
- **`Object`**：Python 对象模型系统，包括对象内存布局控制、Python 类型模型、内建类型的定义及原语实现等。
- **`Execution`**：执行系统，由多个子系统构成。包括但不限于：
  - **`Runtime`**：运行时子系统，提供更高层次的 Python 语言运行时能力
  - **`Builtin`**：内建函数库，提供 Python 中内建函数与内建类型方法的具体实现
  - **`Module`**：模块子系统，提供 MVP 版本的 Python 模块机制实现。虚拟机中的内建模块实现目前也放在此层。
  - **`Interpreter`：字节码解释器系统，负责实际解释执行 Python 字节码。**
- **`Embedder API`**：便于外部用户将虚拟机嵌入到自己的应用当中。

如果你希望了解更多细节，请阅读：
- [VM 系统架构总览](./docs/Saauso-VM-System-Architecture.md)
- [项目结构与阅读路线](./docs/Saauso-Project-Structure-And-Reading-Guide.md)

## 7. 本项目的亮点与创新点

本项目的创新不在于“重新发明一套全新的 VM 理论”，而在于把成熟工业级现代 VM 的设计思想，**迁移、取舍、适配并工程落地**到 Python VM 场景，最终完成一套**轻量、可嵌入、适合教学演示、易于裁剪定制**的 PVM 系统实现。

**项目的工程亮点包括**：
- 采用现代大型 C++ 项目工具链，包括 GN/Ninja、Google Test、ASan、UBSan 与 clang-format，使项目具备**长期可构建、可测试、可验证的工程基础**。
- 提供较完整的样例程序、单元测试和文档体系，**保证本项目是一个具备持续演进能力的系统工程**，而非一个简单的概念原型或课程作业。
- 采用内外双层体系。对外提供 Embedder API，对内保持模块化分层，便于后续继续向 SDK 化、静态库产物与更完整语言特性演进。

**VM 设计上的亮点包括**：
- 采用分层架构设计，将对象表示、内存管理、运行时语义、解释执行与对外嵌入接口解耦，提升系统的可读性、可维护性与可裁剪性。
- 借鉴 HotSpot 的 `Klass-Oop` 思路，把对象实例数据与类型/多态信息解耦，以更清晰地组织 Python 对象模型。
- 借鉴 V8 的 `Smi` 思路，在虚拟机内部复用堆指针位宽表示小整数，减少常见整型值的堆分配开销。
- 借鉴 HotSpot / V8 的 `Handle` 与 `Tracing GC` 思路，围绕 GC 安全引用、对象遍历与托管内存管理建立统一机制，为解释器与嵌入接口提供稳定基础。

## 8. 当前边界与后续演进方向

为了保证系统验收阶段的稳定性，当前版本重点聚焦于核心闭环而非无限扩展功能。

目前已知边界包括：

- 老生代 GC 尚未完成，当前以新生代 Scavenge 为主
- remembered set / write barrier 主路径尚未作为完整分代 GC 方案启用
- 更完整的 Python 语义仍在后续计划中，例如更完整的 importlib/元路径、descriptor 细节、生成器与协程
- 同一 `Isolate` 不保证并发执行安全，跨线程共享需使用 `Locker`
- OOM 暂不承诺可恢复异常语义

这些边界说明的是“系统仍可继续演进”，而不是“项目尚未形成可验收闭环”。  
当前版本已经能够独立支撑对象系统、解释执行、异常、模块导入和 Embedder API 的联动工作流。

## 9. 如何构建

### Release

```shell
gn gen out/release
ninja -C out/release vm
gn gen out/release --export-compile-commands
```

### Debug

```shell
gn gen out/debug --args="is_debug=true"
ninja -C out/debug vm
gn gen out/debug --export-compile-commands
```

### ASan

```shell
gn gen out/asan --args="is_asan=true"
ninja -C out/asan vm
gn gen out/asan --export-compile-commands
```

## 10. 如何测试

### 单元测试构建

```shell
gn gen out/ut --args="is_asan=true"
ninja -C out/ut all_ut
gn gen out/ut --export-compile-commands
```

### 单元测试运行

```powershell
$env:ASAN_SYMBOLIZER_PATH = "D:\LLVM\bin\llvm-symbolizer.exe"
$env:ASAN_OPTIONS = "symbolize=1"
.\out\ut\ut.exe
.\out\ut\embedder_ut.exe
```

项目当前同时提供三类验证材料：

- 单元测试：验证解释器、对象系统、模块系统与 Embedder API 的功能正确性
- Samples：验证宿主程序嵌入脚本引擎的真实使用方式
- 文档体系：验证设计边界、实现思路与工程约束的完整性

## 11. 文档索引

- [AI 贡献指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-AI-Developer-Contributing-Guide.md)
- [代码风格指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md)
- [构建与测试指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Build-and-Test-Guide.md)
- [VM 系统架构总览](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)
- [Embedder API 接入指南](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Embedder-API-User-Guide.md)
- [Embedder API 架构设计](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Embedder-API-Architecture-Design.md)
- [项目结构与阅读路线](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Project-Structure-And-Reading-Guide.md)
