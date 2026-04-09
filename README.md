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
- **教育教学价值**：
  - 相较于 CPython 数十万行代码的体量，本项目的核心代码只有 3 万多行。
  - 因此本项目更适合作为《编译原理》等系统类课程的教学用具，在课堂上进行演示或讲解。
- **工程与实际应用价值**：
  - 本项目可作为办公软件、游戏引擎等宿主程序内的轻量级脚本引擎，而不是只能作为独立命令行解释器存在。
  - 本项目采用现代 C++ 语言进行开发，且实现了清晰的分层架构，十分便于开发人员对虚拟机进行裁剪、修改定制或二次开发。
- **人才培养价值**：
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

**这个例子用于说明**： 本项目已支持循环、判断、递归等基础的控制流。  

**出处**：[test/unittests/interpreter/interpreter-functions-unittest.cc](test/unittests/interpreter/interpreter-functions-unittest.cc) 中的 `FibRecursion`

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

**出处**：[test/unittests/interpreter/interpreter-custom-class-core-unittest.cc](test/unittests/interpreter/interpreter-custom-class-core-unittest.cc) 中的 `MagicMethodCanBeOverrided`

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

**出处**：[test/unittests/interpreter/interpreter-exceptions-unittest.cc](test/unittests/interpreter/interpreter-exceptions-unittest.cc) 中的 `TryExceptFinallyWithExceptionEmitByVm`


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

## 6. 虚拟机整体架构介绍

本项目的架构设计借鉴了现代工业级 VM 系统（如 V8、HotSpot），目标是让对象表示、运行时语义、执行引擎与对外接口边界清晰、职责单一，便于进行长期拓展和维护。

![Architecture](./logo/architecture-light.png#gh-light-mode-only)
![Architecture](./logo/architecture-dark.png#gh-dark-mode-only)


### 6.1 VM 内部分层

- `utils`：纯工具层，不依赖 VM 上层语义
- `handles`：句柄与 GC 安全持有
- `objects`：对象表示、布局、字段、GC 遍历与 vtable 槽位骨架
- `execution`：`Isolate`、异常状态与执行门面
- `builtins/runtime`：内建函数与跨类型高层语义 helper
- `interpreter`：字节码调度、调用协议、栈帧管理与异常展开
- `modules`：导入系统、路径解析、模块缓存、加载与注册

### 6.2 Embedder API 三层结构

- `include/`：公共 ABI 与对外头文件
- `src/api/`：桥接层，负责把公共 API 翻译到 VM 内部能力
- `src/**`：VM 内部实现，包括对象系统、运行时、解释器与堆管理

这种分层的直接收益是：

- 降低模块间耦合，避免“所有逻辑堆在一个类里”
- 让脚本引擎对外接口与 VM 内部实现解耦
- 便于后续演进为更完整的解释器或接入其他执行器

如果你希望继续深读架构，请阅读：

- [VM 系统架构总览](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-VM-System-Architecture.md)
- [项目结构与阅读路线](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Project-Structure-And-Reading-Guide.md)

## 7. 项目的工程亮点与创新点

本项目的创新不在于“从零发明一套全新的虚拟机理论”，而在于把成熟工业级 VM 的设计思想，适配并工程落地到 Python 虚拟机场景中。

具体亮点包括：

- 兼容 CPython 3.12 字节码模型，把重点放在运行时和解释器实现，而不是重复设计一套前端字节码格式
- 使用 `co_exceptiontable` 与 zero-cost unwind 主路径处理异常，而不是只靠简化版异常栈模型
- 实现面向 CPython 3.12 调用形态的解释器调用协议，覆盖函数、方法与参数绑定主路径
- 通过对象系统、模块系统、异常系统、内存管理与 Embedder API 形成完整工程闭环
- 采用公共 ABI 层、桥接层、VM 内核三层结构，使项目同时具备教学可读性与工程可维护性
- 提供样例程序、单元测试和文档体系，证明项目不是停留在概念验证阶段

如果用一句话概括本项目的亮点，可以表述为：

> S.A.A.U.S.O 的核心价值，在于将 HotSpot、V8、CPython 等成熟系统中的设计思想，迁移并实现为一个可运行、可验证、可嵌入、可教学展示的 Python 虚拟机系统。

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
