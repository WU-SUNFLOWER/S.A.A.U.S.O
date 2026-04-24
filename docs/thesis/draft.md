# 一个轻量级 Python 虚拟机的设计与实现

# 摘要

# 第1章 绪论

## 1.1 课题背景

Python 语言凭借简洁的语法、较低的上手门槛以及丰富的第三方生态，已被广泛应用于 Web 开发、数据分析、人工智能、自动化运维等多个领域。作为一种动态类型高级语言，Python 程序通常并不直接运行在底层硬件之上，而是依赖 Python 虚拟机（Python virtual machine，PVM）完成编译、加载与执行。

虚拟机技术是现代高级编程语言的核心，它主要包括前端和后端两大部分。前端中由编译器（compiler）将高级语言程序的源代码（source code）翻译成中间字节码（bytecode）。后端中由解释器（interpreter）负责执行字节码。此外，后端还提供垃圾回收器（garbage collector）和编程语言运行时（runtime）等核心模块。这种机制使得 Python 或其他运用该项技术的高级编程语言，可以在任何安装了相应虚拟机软件的计算机系统上运行，而在源代码层面几乎不需要进行任何修改。

目前，最主流、最成熟的 Python 实现是官方维护的由 C 语言实现的 CPython。经过长期演进，CPython 在兼容性、稳定性和生态支持方面都已经非常完善。但从教学演示、轻量嵌入和定制化开发的角度来看，直接研究或裁剪一个大规模工业实现，往往存在两个现实困难。

一方面，CPython 作为长期维护的工业项目，且受限于 C 语言自身的表达与抽象能力，代码体量大、内部机制多，理解和修改的门槛较高。对于希望聚焦编程语言运行时核心机制的教学或实验场景而言，这种复杂度并不总是必要的。另一方面，许多嵌入式脚本场景更关心“够用、易嵌入、便于裁剪和二次开发”，而不是完整覆盖全部 Python 生态。在这些场景中，一个结构清晰、目标聚焦的轻量级 PVM 后端实现，具有一定的研究与工程价值。

基于上述背景，本文提出使用现代 C++20 语言设计并实现一个轻量级 Python 虚拟机后端系统，即 S.A.A.U.S.O VM。该系统以教学与嵌入场景为主要目标，兼容 CPython 3.12 字节码模型，并支持 Python 语言的一个核心功能子集。本文中将重点讨论该系统中对象系统、解释器、异常机制、模块系统、内存管理以及嵌入接口等关键部分的设计思路与工程实现；相比直接讨论一个完整工业实现，围绕这些核心模块展开设计与实现，更有利于说明编程语言虚拟机的内部运行原理。

本文所实现的 S.A.A.U.S.O VM 已开源，相关链接见附录 A。

## 1.2 国内外研究现状

### 1.2.1 国外研究现状

在 Python 实现与编程语言虚拟机领域，国外已经积累了较为丰富的研究和工程成果，形成了多条有代表性的技术路线。

首先，在 Python 语言实现方面，CPython 仍然是事实上的标准实现，强调语言兼容性、稳定性和生态完整性。围绕 CPython 的替代或补充方案也持续出现。例如，PyPy 通过 JIT 技术提升运行效率，在动态语言性能优化方面具有代表性；MicroPython 面向资源受限设备，在体量和部署门槛方面具有明显优势；Codon 则尝试通过静态编译路径获得更高的执行效率。这些方案分别在兼容性、轻量化、运行效率和适用场景之间作出了不同取舍。

其次，从更广泛的编程语言虚拟机技术研究领域来看，Strongtalk、HotSpot JVM 和 V8 JavaScript Engine 等工业级虚拟机的研究成果对后续语言运行时设计产生了深远影响。诸如对象模型分层、句柄系统、分代垃圾回收、内联缓存和运行时优化等思想，已经成为现代虚拟机设计中的重要参考。这些成果虽然并非直接面向 Python 语言，但它们在对象表示、执行模型和内存管理方面的设计经验，对构建新的 PVM 系统具有较强借鉴意义。

总体来看，国外相关研究已形成较为完整的生态：既有追求工业成熟度与语言完整性的实现，也有面向特定硬件平台或高性能需求的变体。这为本文选题提供了良好的比较对象和设计参考。

### 1.2.2 国内研究现状

相较于国外较成熟的虚拟机研究与开源生态，国内在 Python 语言实现和 PVM 方向上的公开研究相对较少，且整体更偏应用导向。

一类工作主要集中在 Python 的应用层使用，例如许多企业和高校会将 Python 用于 Web 服务、自动化脚本或数据处理任务，研究重点通常不在 PVM 本身。另一类工作则更多围绕 CPython 的扩展开发或源码剖析展开，这些工作对理解 Python 运行机制具有帮助，但通常并不以"从零设计并实现一个新的 PVM"为目标。

在更广泛的编程语言虚拟机领域，国内部分互联网科技企业也会围绕具体业务场景定制专用虚拟机或运行时系统，例如字节跳动的 PrimJs、阿里巴巴的 DragonwellJDK 等。不过整体而言，这类成果往往偏工程内部使用，公开资料有限；结构清晰、适合教学或定制化修改的开源实现并不多见。

总体而言，国内在“轻量级、可嵌入、便于教学与定制化修改的 Python 虚拟机后端”这一交叉方向上，公开可参考的系统性实践仍然较少，这也进一步说明了本文工作的实际意义。

## 1.3 课题研究的内容及意义

综合来看，现有 Python 实现方案往往各有侧重：有的强调完整兼容性，有的强调运行效率，有的强调在极小硬件上的可部署性，但在“轻量化、便于嵌入、结构清晰、相对易于教学演示和二次开发”这几个维度上，同时兼顾的公开实现并不多见。

基于此，本文以教学与嵌入场景为主要目标，设计并实现一个名为 S.A.A.U.S.O 的轻量级 PVM 系统。该系统以兼容 CPython 3.12 字节码模型、支持 Python 语言的常用核心功能为目标，在保证核心功能可运行、可测试的前提下，尽量保持项目规模可控、模块边界清晰，并提供面向宿主程序的嵌入接口。

### 1.3.1 研究内容

- 设计并实现一个基于 C++20 的轻量级 PVM，支持 Python 语言的核心功能子集，并兼容 CPython 3.12 字节码执行模型中的关键路径；
- 设计并实现对象系统、字节码解释器、异常机制、模块系统与内存管理等核心模块，使系统形成可运行的最小闭环；
- 提供面向宿主程序的 Embedder API，使该虚拟机能够作为脚本引擎嵌入 C++ 应用；
- 在保证功能正确性的前提下，尽量保持系统分层清晰、实现可读、工程规模可控，便于教学演示、后续二次开发以及按需裁剪。

### 1.3.2 研究目标

本文希望围绕一个轻量级 PVM 的设计与实现，回答以下几个问题：

- 如何在不完整复刻 CPython 的前提下，实现对 Python 核心功能子集的可用支持；
- 如何围绕字节码解释执行、对象模型和运行时机制，构建一个具有清晰层次结构的 PVM；
- 如何为宿主程序提供简洁可用的嵌入接口，使该系统不仅能独立运行脚本，也能作为脚本引擎集成到其他应用中；
- 如何通过测试与样例验证该系统在正确性、可嵌入性和工程可演进性上的可行性。

### 1.3.3 研究意义

需要说明的是，S.A.A.U.S.O VM 的目标并不是替代已经高度成熟的 CPython，而是在特定目标下提供一种更轻量、更易理解、也更适合教学演示和定制化修改的系统实现。从这个角度看，本文工作主要具有以下几方面意义。

#### （1）教育教学价值

相较于体量庞大的工业级 VM，一个结构更聚焦、体量更可控的 PVM 一般更适合作为教学材料。通过 S.A.A.U.S.O VM，可以更直观地展示对象系统、字节码解释执行、异常传播、模块加载和内存管理等核心机制，有助于辅助《编译原理》《操作系统》等计算机基础系统开发类课程的教学。

#### （2）工程与实际应用价值

S.A.A.U.S.O VM 采用现代 C++20 实现，并提供面向宿主程序的嵌入接口。对于不需要完整 Python 生态、但希望在自身应用中引入脚本能力的场景，该系统可以作为一种轻量级脚本引擎方案。同时，清晰的模块边界也有利于嵌入方的开发者对其进行裁剪、修改和定制。

#### （3）人才培养价值

从实现过程看，PVM 的设计与实现并不是单一模块开发，而是涉及编程语言原理、数据结构、面向对象设计、内存管理、软件工程和系统调试等多方面知识。完成这样一个系统，对于训练计算机专业本科毕业生的复杂系统设计能力、工程实现能力和问题定位能力都具有较强的综合价值。

## 1.4 本文主要内容与结构

本文共分为 6 章，章节的主要内容如下：
- 第1章 绪论：给出研究背景、国内外研究现状、研究内容与意义，以及论文结构安排；
- 第2章 相关理论与关键技术基础：介绍与实现密切相关的字节码模型、栈帧、对象系统、异常与模块机制等；
- 第3章 系统总体设计：阐述 S.A.A.U.S.O VM 的总体架构、模块划分与关键数据结构/流程；
- 第4章 关键模块实现：分别说明 S.A.A.U.S.O VM 中解释器、对象模型、内存管理、异常处理、模块系统与嵌入接口的工程实现；
- 第5章 测试与结果分析：给出正确性与兼容性测试、性能与资源数据，并进行分析讨论；
- 第6章 总结与展望：总结工作与不足，给出未来改进方向。

---

# 第2章 PVM 理论基础与关键技术

本章围绕 S.A.A.U.S.O VM 的实现目标，概述轻量级 PVM 所涉及的理论基础与关键技术。本章中不会完整复述 Python 语言的全部细节，而只讨论与本文系统实现直接相关的执行模型、函数与栈帧、对象系统、异常机制和模块机制，从而为第 3 章和第 4 章提供必要的理论基础。

## 2.1 PVM 执行原理概述

### 2.1.1 CPython 执行模型概述

Python 程序的执行通常分为两个阶段：前端将源代码以函数为单位（脚本中的顶层代码会被视作一个特殊的函数）翻译为代码对象（code object），其中包含字节码指令序列、常量表和符号名表等关键信息；后端虚拟机再对其中的字节码指令进行解释执行。本文主要关注后者。

从指令的执行方式看，PVM 本质上是一种虚拟的栈机（stack machine）。它维护操作数栈，并按照“取指、译码、执行”的顺序逐条处理字节码指令。在 CPython 的执行模型中，每条字节码指令由操作码和操作参数组成，其职责覆盖常量加载、变量读写、运算、函数调用、属性访问和控制流跳转等核心语义。

例如对于代码 2-1 中给出的简单例子：

```python
x = 1
y = 2
z = 3
x + y * z
```
代码 2-1

编译器前端生成的字节码指令序列如代码 2-2 所示：
```
地址       操作码                  参数
  2      LOAD_CONST               0 (1)
  4      STORE_NAME               0 (x)

  6      LOAD_CONST               1 (2)
  8      STORE_NAME               1 (y)

 10      LOAD_CONST               2 (3)
 12      STORE_NAME               2 (z)

 14      LOAD_NAME                0 (x)
 16      LOAD_NAME                1 (y)
 18      LOAD_NAME                2 (z)
 20      BINARY_OP                5 (*)
 24      BINARY_OP                0 (+)
```
代码 2-2

PVM 执行这段代码时，会通过 `LOAD_CONST`、`STORE_NAME`、`LOAD_NAME` 和 `BINARY_OP` 等指令分别完成常量加载、存储变量、读取变量和二元表达式运算。这说明 PVM 并不直接“理解”高级语言语法，而是通过解释字节码间接实现语言语义。

但一个可运行的 PVM 也不只是维护操作数栈。为了支持函数、异常传播和模块加载，系统还必须维护栈帧（stack frame）、命名空间、异常状态以及运行时服务。后文会对这些概念展开探讨。

### 2.1.2 Python 中的命名空间与变量绑定机制概述

在 Python 中，变量更接近于“名称到对象的绑定关系”，而不是某个固定地址上的存储单元。也就是说，变量读写的本质是在某个命名空间中查找、建立或更新绑定。

从 PVM 的角度看，最常见的命名空间包括函数调用形成的局部命名空间、模块或脚本顶层对应的全局命名空间，以及提供内建函数的内建命名空间；闭包场景下还需要通过额外的单元对象维持自由变量的跨栈帧可见性。

例如，在代码 2-3 中，`add` 函数和变量 `c` 属于全局命名空间，`add` 函数中的形参 `a`、`b` 和局部变量 `result` 属于调用该函数而产生的局部命名空间，`print` 函数属于内建命名空间。

```python
def add(a, b):
  result = a + b
  return result

c = add(1, 2)
print(c)
```
代码 2-3

这意味着，PVM 中在读写变量时，必须结合局部、全局、内建和闭包环境完成名称解。具体到 CPython 3.12，不同命名空间对应 `LOAD_NAME/STORE_NAME`、`LOAD_FAST/STORE_FAST`、`LOAD_GLOBAL/STORE_GLOBAL` 与 `LOAD_DEREF/STORE_DEREF` 等不同指令，它们的机制在 2.3.2 中会结合栈帧进一步展开。

## 2.2 Python 中基本的控制流

控制流的本质，是决定程序下一步执行哪一条指令。PVM 在解释执行过程中需要维护程序计数器，用于标识下一条待执行字节码的位置。顺序执行时，程序计数器按字节码顺序推进；而遇到条件分支、循环或异常等情况时，程序计数器则会被跳转相关的字节码显式修改。

因此，Python 中的 `if`、`while`、`break` 和 `continue` 等语句，在 PVM 内部最终都会落实为若干条件跳转与无条件跳转指令的组合。代码 2-4 中给出了一个基本的 `while` 循环例子。

```python
while condition:
    i += 1
print(i)
```
代码 2-4

编译器前端生成的字节码指令序列，核心内容代码 2-5 所示：
```
地址       操作码                      参数
  2      LOAD_NAME                0 (condition)
  4      POP_JUMP_IF_FALSE        6 (to 18)

  6      LOAD_NAME                1 (i)
  8      LOAD_CONST               0 (1)
 10      BINARY_OP               13 (+=)
 14      STORE_NAME               1 (i)

 16      JUMP_BACKWARD            8 (to 2)

 18      后续代码略...
```
代码 2-5

分析编译结果可知，循环在 PVM 中并不是独立机制，而是“条件判断 + 跳转”的组合：前者依赖 `POP_JUMP_IF_FALSE` 等条件跳转指令，后者依赖 `JUMP_BACKWARD` 等无条件跳转指令。

与此同时，Python 的控制流判断还涉及真值语义（truthiness），即需要将 Python 程序中的具体对象动态地解释为真或假。因此，PVM 不仅需要支持跳转指令本身，还必须让这些指令与对象真值判定等运行时能力协同工作。

## 2.3 Python 中的函数

函数是 Python 程序中最重要的组织单位之一。对 PVM 而言，函数机制并不只是“执行一段可复用代码”这么简单，它还牵涉到代码对象创建、运行时函数对象构造、参数绑定、栈帧切换以及闭包变量访问等多个环节。

### 2.3.1 简单函数的创建

不同于 C/C++ 等静态语言，在 CPython 的执行模型中，函数的静态部分和动态部分是分开的：编译阶段先为函数体生成代码对象；运行阶段，当程序执行到到函数定义语句时，解释器再通过 `MAKE_FUNCTION` 指令把代码对象包装为可调用函数对象，并通过 `STORE_NAME` 指令绑定到当前命名空间。

下面给出一个具体的函数创建例子来进一步说明，见代码 2-6。

```python
def add(a, b):
    return a + b

result = add(1, 2)
```
代码 2-6

编译器前端生成的字节码指令序列中，创建 `add` 函数对象的过程如代码 2-7 所示。
```
地址       操作码                      参数
 2      LOAD_CONST               0 (<code object add at 0x5b596d3b75e0>)
 4      MAKE_FUNCTION            0
 6      STORE_NAME               0 (add)
```
代码 2-7

从这段字节码可以看到，`add` 对应的代码对象先出现在外层代码对象的常量表中，随后由 `MAKE_FUNCTION` 在运行时构造出真正的函数对象。

为了说明动态创建函数这一机制的必要性，代码 2-8 中给出一个带默认参数函数的例子。
```python
default_k = 3
def linear_f(x, b, k = default_k):
    return k * x + b
```
代码 2-8

这个过程是如何实现的呢？分析代码 2-9 中给出的编译器前端生成的字节码指令序列，不难找到答案。
```
地址       操作码                      参数
  6      LOAD_NAME                0 (default_k)
  8      BUILD_TUPLE              1
 10      LOAD_CONST               1 (<code object linear_f at 0x57ba48f76b90>)
 12      MAKE_FUNCTION            1 (defaults)
 14      STORE_NAME               1 (linear_f)
```
代码 2-9

从这段指令中可以看到，创建带默认值参数的函数时，默认值元组是在运行时动态确定的，再由 `MAKE_FUNCTION` 与代码对象一起绑定进函数对象。因此，Python 函数不能被简单视为静态代码片段，而必须建模为运行阶段创建的对象，且具备绑定运行时产生的数据的能力。第 4 章中将会给出函数对象在本文系统中的实际实现。

### 2.3.2 栈帧与函数调用的原理

Python 中的函数调用通常通过 `CALL` 等字节码指令发起。每次调用发生时，PVM 都需要为被调函数创建新的栈帧，用于保存程序计数器、操作数栈、局部变量环境以及与当前函数相关的上下文信息；函数返回时，再弹出该栈帧并把控制权交还给调用方。因此，函数调用关系在 PVM 中表现为一组栈帧的入栈和出栈过程。

栈帧的意义并不只是保存临时数据，它本质上承担了“局部执行上下文”的职责。局部变量访问、函数返回以及异常展开等机制，都建立在这一点之上。

从实现角度看，若让所有局部变量都通过字典按名字查找，执行代价会过高。因此，PVM 通常会在栈帧中引入一个槽位区（定长数组）：编译阶段为局部变量分配槽位，运行阶段再通过 `LOAD/STORE` 指令参数中的槽位下标直接读写变量。

在 CPython 3.12 的执行模型中，对字典和槽位区两种方案进行了结合。首先，对于单个 Python 文件的顶层代码，也就是可视作“根函数”的那部分代码，因为其中的顶层变量可能会通过变量符号名被暴露给其他 Python 文件，因此栈帧通常会提供一个被称为 `locals` 的字典用于保存这些顶层变量。而要以变量符号名为键访问这些顶层变量，就需要通过 `LOAD_NAME` 和 `STORE_NAME` 等字节码指令实现。

这里进一步说明 `LOAD_NAME` 与 `STORE_NAME` 的语义差异。对于 `STORE_NAME` 而言，其语义是在当前按名字解析的局部命名空间中建立或更新绑定关系，因此它会直接把值写入当前栈帧的 `locals` 字典。然而，`LOAD_NAME` 的语义并不是“只查当前 `locals`”即可结束：当名称在当前栈帧的 `locals` 字典中不存在时，PVM 还需要继续在 `globals` 以及内建命名空间中进行查找。这样设计的原因在于，在 CPython 当前的执行模型中，可能会复用这条指令用于查找模块级全局变量或内建函数。

相对地，对于一般 Python 函数中的局部变量，则采用了上文介绍的槽位区方案。这个槽位区在 CPython 的源代码中被称为 `localsplus` 数组。另外，Python 函数中的形参以及闭包场景下出现的自由变量，同样会被安排在槽位区中进行维护。相应地，`LOAD_FAST`、`STORE_FAST`、`LOAD_DEREF`、`STORE_DEREF` 等字节码指令就是直接按槽位下标访问对应变量值的。需说明的是，关于闭包的实现原理，在 2.3.3 中会进一步讨论。

与此同时，栈帧中还会预留一个 `globals` 字段用于指向当前函数代码使用的全局命名空间字典。若函数内部需要读取其所在模块中的全局名称，则通过 `LOAD_GLOBAL` 指令，PVM 会先查该函数绑定的全局命名空间，再查内建命名空间作为兜底；若函数体显式对全局变量赋值，则 PVM 会通过 `STORE_GLOBAL` 指令落到该函数绑定的全局命名空间之中。

上文中说过，顶层代码中的变量会直接被放进根函数栈帧的 `locals` 字典中维护，那么为什么还需要引入 `globals` 的概念呢？这一点可以通过代码 2-10 中的一个简单例子更直观地解释。

```python
# mod_a.py
x = 10
def func():
    print(x)

# mod_b.py
from mod_a import func
x = 999
func()
```
代码 2-10

在这个例子中，我们假设有两个模块 `mod_a.py` 和 `mod_b.py`。当 PVM 执行 `mod_a.py` 的顶层代码时，会为该模块准备一个模块级全局命名空间。在 CPython 3.12 的执行模型中，当前根函数对应栈帧中的 `locals` 与 `globals` 字段均直接指向这一个字典。当 `MAKE_FUNCTION` 创建 `func` 时，会把栈帧中 `globals` 字段所指向的该字典一并绑定进新生成的函数对象中。于是，当我们在 `mod_b.py` 中调用 `func` 时，PVM 首先会将当前栈帧中的 `globals` 字段指向与该函数绑定的全局命名空间。接下来，因为函数读取变量 `x` 的操作是通过 `LOAD_GLOBAL` 字节码指令完成的，所以 PVM 会在当前栈帧中 `globals` 所指向的全局命名空间中进行查找，从而得出的输出结果为 10 而非 999。

综上所述，PVM 栈帧需要同时提供 `locals`、`globals` 和 `localsplus` 三类环境。它们的职责不同，不能相互替代；这也是后文实现栈帧和变量读写指令的直接依据。

### 2.3.3 函数闭包

不同于 C/C++，Python 中支持嵌套定义函数，因此进一步会延伸出闭包（closure）机制。闭包是 Python 词法作用域规则的重要体现。

首先通过代码 2-11 中的例子，简要说明 Python 语言中闭包的概念和特点。
```python
def outer():
    x = 10
    def inner():
        print(x)
    return inner
```
代码 2-11

在这段代码中，内部函数 `inner` 对外部函数 `outer` 中的局部变量 `x` 形成引用。因此 `inner` 被称为闭包函数（closure function）；变量 `x` 则被称为 `inner` 的自由变量（free variable）。

站在 PVM 的视角，要实现闭包机制，至少需要解决几个核心问题：
（1）PVM 需要能够区分普通的函数局部变量和自由变量；
（2）PVM 需要保证自由变量在外部函数返回后不会立即消失，而是继续随内部函数一起存活；
（3）PVM 需要保证闭包函数运行时，观察到的是自由变量的当前实际值，而非闭包函数创建时自由变量的副本值；
（4）PVM 需要保证外部函数和内部闭包函数均支持对自由变量进行读写。

下面为了进一步说明 CPython 3.12 执行模型中是如何解决这几个核心问题的，再给出代码 2-11 对应的删节字节码指令，见代码 2-12。

```
outer 函数的字节码指令序列：
地址       操作码                      参数
  0      MAKE_CELL                1 (x)
  4      LOAD_CONST               1 (10)
  6      STORE_DEREF              1 (x)
  8      LOAD_CLOSURE             1 (x)
 10      BUILD_TUPLE              1
 12      LOAD_CONST               2 (<code object inner>)
 14      MAKE_FUNCTION            8 (closure)

inner 函数的字节码指令序列：
地址       操作码                      参数
  0      COPY_FREE_VARS           1
 14      LOAD_DEREF               0 (x)
```
代码 2-12

从这段代码中可以看出，闭包机制并不是依靠某一条孤立指令完成的，而是由多条字节码指令配合形成一条完整链路。首先，在外层函数 `outer` 中，`MAKE_CELL` 字节码指令会在变量 `x` 所对应的局部变量槽位中新创建一个单元对象（cell object）；随后，`STORE_DEREF` 字节码指令并不是把常量 `10` 直接写入 `x` 对应的槽位，而是写入该槽位的单元对象当中。这意味着从这一时刻开始，`x` 在运行时就已经不再以“普通局部变量值”的形态存在，而是变成了一个可被多个函数共享的间接引用单元。与此同时，这部分字节码指令表明，Python 代码中普通局部变量和自由变量是在编译阶段完成区分的，这就首先回答了问题（1）。

接下来，在内部函数 `inner` 被创建之前，`LOAD_CLOSURE` 指令会把 `x` 对应的单元对象压入操作数栈。随后，`BUILD_TUPLE` 字节码会将操作数栈顶的全体单元对象打包成一个 Python 元组。最后，被编译器前端添加特殊参数标记的 `MAKE_FUNCTION` 指令会在创建函数对象时，会把这个装有单元对象的元组绑定进内部函数。这里最关键的一点在于：内部函数拿到的并不是 `x` 当前变量值的一份副本，而是对同一个单元对象的引用。也正因为如此，当外层函数 `outer` 后续返回时，只要这个函数对象仍然存活，它就能够继续持有该单元对象，从而解决问题（2）。

当内部函数 `inner` 真正被调用时，`COPY_FREE_VARS` 指令又会把函数对象里保存的这些单元对象重新注入新建立栈帧中，使该栈帧的局部变量区域继续持有同一个单元对象。随后，`LOAD_DEREF` 才真正执行对自由变量的读取操作。换言之，内部函数运行时读取到的，并不是当初创建函数对象时复制下来的一份静态值，而是通过单元对象间接解析出来的“当前值”。因此，无论是外层函数还是内部函数，只要它们操作的是同一个单元对象，就都能够观察到对该变量所做的更新。这就解释了 PVM 是如何实现问题（3）与（4）的。

综上所述，是一套需要由编译器前端、单元对象间接层和专用字节码指令共同协作完成的完整机制。


### 2.3.4 内建函数

除了用户定义的函数之外，PVM 还要提供 `print`、`len`、`isinstance` 等内建函数。这些函数通常由 VM 的 native 语言实现，并预先注册到内建命名空间中，是语言语义与虚拟机系统之间的重要桥梁。

对 PVM 来说，关键不只是“写出几个 native 函数”，而是要把它们与普通 Python 函数统一建模为可调用对象。编译器前端只需生成通用的调用字节码，然后在运行阶段由 PVM 进行分流处理。这一点将在 4.5.5 中对应落地。

## 2.4 Python 中的面向对象机制

Python 是一门面向对象的动态语言。对象系统不仅影响类和实例本身，还会影响属性访问、方法调用、内建类型表示、继承与多态等一系列运行时行为。对于 PVM 实现来说，对象系统往往是贯穿解释器、运行时和内存管理的核心基础设施。

### 2.4.1 综述

在 Python 中，万物皆对象。数字、字符串、列表、函数、模块，都可以统一看作 Python 对象。特别地，不同于 C++，Python 中类（class）本身也是一种对象，即类型对象（type object）。Python 中的"万物皆对象"的设计使得 Python 具有较强的灵活性，但也意味着 PVM 需要建立一套统一的对象表示机制，来同时描述“对象实例”和“对象类型”。

因此对于 PVM 的设计与实现而言，必须保证对象模型并不只是语法层面的“类和对象”概念，而是整个服务于 VM 上层逻辑的统一数据组织方式。在 PVM 内部，解释器执行过程中操作的栈值、运行时中传递的函数对象、模块系统中维护的模块对象，最终都要落在统一的对象表示之上。

### 2.4.2 基本内建类型

一个最小可用的 PVM 后端，通常需要先支持若干基础内建类型，才能承载更高层语言语义。对 Python 而言，这些类型至少包括 `object`、`int`、`float`、`str`、`tuple`、`list`、`dict` 等。它们分别承担了不同角色：`object` 是一切 Python 类型的祖先类型；数值与字符串类型支撑最常见的数据表达；元组、列表和字典则是函数创建、函数调用和模块机制等虚拟机核心逻辑中频繁使用的基础容器。

这些内建类型并不是简单的数据结构集合。PVM 还应当为它们实现各自的运行时能力，例如通过下标访问列表元素、对整型或浮点型进行算术计算、获取字符串长度等。因此，在 PVM 中实现内建类型，既要设计合理的对象布局，也要实现这些内建类型的运行时能力。

### 2.4.3 类的封装

封装的核心含义，是将对象状态与围绕该状态运行的方法组织在一起。Python 同样支持这一基本思想：用户可以在类中定义属性和方法，再由实例对象持有具体状态，通过方法对外提供行为。

不过，Python 的封装方式与 C++、Java 等静态语言并不完全相同。首先，Python 并不提供严格的 `public`、`private` 等访问控制修饰符。其次，Python 中类对象本身在运行时具有高度动态性；例如，程序可以向一个类中动态添加新的方法，也可以将类中的某个方法替换为另一个函数。再次，Python 允许在程序运行过程中针对某个对象实例单独添加附加属性，因此实例状态与类行为通常需要被拆分到不同的属性容器中维护。

这意味着 PVM 在实现对象系统时，不能简单照搬静态语言中“字段偏移在编译阶段固定、方法入口在编译阶段预先绑定”的对象模型。首先，PVM 需要采用字典容器的方式分别存储对象实例属性和类属性。在 Python 语言中，前者和后者分别对应于 Python 对象自身和类对象持有的 `__dict__` 字典。其次，在属性读取时，PVM 需要按一定顺序进行解析：通常先查实例自身持有的属性，再查类型对象及其继承链上的类属性，必要时再进入补救机制。只有这样，PVM 才能正确支持 Python 运行时的动态封装语义。

此外，方法绑定也是 Python 动态封装行为的一部分。下面通过代码 2-13 中的例子进行说明。
```python
class MyClass:
  def __init__(self, value):
    self.value = value

  def foo(self):
    print("I can receive self argument")
    print(self.value)

def bar():
  print("I can't receive anything")

instance = MyClass(2026)
instance.bar = bar
instance.foo()
instance.bar()
```
代码 2-13

在这个例子中，当虚拟机在查找 `instance` 对象的 `foo` 属性时，因为该属性来自实例的祖先类，且该属性的值为函数对象，因此 PVM 会在运行时把它与当前实例组合为“绑定方法”。当 Python 程序调用"绑定方法"时，PVM 会自动传入相应的对象实例作为方法函数的 `self` 参数。相反地，因为 `bar` 函数只是临时被写入实例自己的属性字典中，那么它会被 PVM 视作一个普通属性值，并不会生成"绑定方法"。

因此，PVM 中的对象系统不能只实现“属性查找”，还必须进一步区分实例属性、类属性与方法绑定这三类不同语义，否则就无法正确实现 Python 语言的动态封装行为。

### 2.4.4 类的继承

继承允许一个类在已有类的基础上扩展行为和结构，是 Python 面向对象机制中的重要组成部分。通过继承，子类可以复用父类的属性和方法，也可以重写父类方法形成新的行为。

除了单继承之外，Python 还支持多继承。多继承带来的核心问题是：当多个父类中存在同名属性或方法时，PVM 应当按照什么顺序进行查找。为此，Python 引入了方法解析顺序（Method Resolution Order，MRO）的概念，并采用 C3 线性化算法生成类层次结构中的查找顺序。

C3 线性化算法由 Kim Barrett, Bob Cassels 等人在论文《A Monotonic Superclass Linearization for Dylan》中提出。从思想上看，C3 线性化算法并不是简单地把多个父类线性拼接起来，而是要在保持局部继承顺序的前提下，逐步选择一个“合法的下一个类型”加入最终 MRO 序列。其核心过程可以简要描述为代码 2-14 中的伪代码。

```python
while 仍有未处理的父类MRO序列:
  在各序列的头元素中选择一个“不出现在其他序列尾部”的合法元素
  将该元素加入结果MRO序列
  从所有序列中删除该元素
```
代码 2-14

从 PVM 实现角度看，这意味着类型创建过程不能只完成“分配一个类型对象”这么简单，还需要在类的创建过程中正确计算并保存该类的 MRO 序列。与此同时，PVM 要让 MRO 序列直接参与 PVM 中类属性查找、方法解析等逻辑。

### 2.4.5 类的多态

传统意义上的多态指相同的调用接口在不同对象上表现出不同的行为。但不同于 C++ 和 Java 中基于接口类的多态，Python 中的多态除了来源于传统的继承和方法重写，也和该语言的鸭子类型特征密切相关。也就是说，在 Python 中允许调用方在静态编译阶段不限定和不知晓对象的静态类型，而仅在运行阶段尝试让目标对象响应某种操作或请求。

例如在代码 2-15 的例子中，尽管 `do_say` 函数完全不了解传递进来的对象类型，但由于 `Cat` 类和 `Dog` 类都具备不需要传入任何参数的 `say` 方法，因此将两者的实例对象传入 `do_say` 函数，程序均能成功运行。

```python
def do_say(animal):
  animal.say()

class Cat:
  def say(self):
    print("meow")

class Dog:
  def say(self):
    print("woof")

cat_instance = Cat()
dog_instance = Dog()

do_say(cat_instance)
do_say(dog_instance)
```
代码 2-15

从这一意义上说，Python 中的多态本质上依赖于 2.4.3 中讨论的运行时阶段的属性查找与方法绑定。对于用户自定义类而言，对象实例属性、类属性以及继承链上的属性查找结果，会共同决定调用一个方法最终表现出的行为。因此，语言层面的多态并不要求 PVM 在编译阶段就把某个调用点与具体实现预先绑定，而是要求虚拟机在运行阶段保留足够的动态解析能力。

但对实际的 PVM 而言，仅仅保留语言层面的动态性还不够。若将所有对象行为都统一退化为字典查询与方法绑定，虽然能够较自然地贴近 Python 语义，但在 PVM 内部会带来较高的运行时开销。例如，假设每次执行整数加法运算时，PVM 都要在 `int` 类型中查找一次 `__add__` 方法，则这种性能开销显然是难以接受的。

基于此，在后续介绍本文所实现的 S.A.A.U.S.O VM 时，需要进一步区分“两层多态”：一层是面向 Python 语言语义的动态属性查找与方法绑定，另一层是面向 VM 内部执行效率的核心行为分派。如何在不破坏前者的前提下，引入后者作为更加高效的调度机制，将成为第 4 章对象系统实现部分需要回答的问题。

## 2.5 Python 中的异常机制

站在 PVM 的角度来看，异常机制本质上是一种特殊的控制流机制。与普通的顺序执行或条件跳转不同，异常会在运行时打断当前正常推进的解释器流程，并尝试将控制权转移到 Python 代码中距离异常发生点最近的异常处理器（exception handler）处。因此，对 PVM 而言，异常机制并不是附属功能，而是解释器主执行逻辑的组成部分。

Python 的异常处理以异常对象（如 `RuntimeError`、`TypeError` 等）为中心。当程序执行过程中虚拟机内部抛出异常，或用户显式执行 `raise` 语句时，PVM 会创建一个异常对象，并作为虚拟机内部的异常状态记录下来。

之后，PVM 需要查找并将控制流转向 Python 程序中与异常发生点相匹配的异常处理器。这个过程是 PVM 通过栈展开（stack unwinding）完成的。栈展开的过程可以用代码 2-16 中的伪代码进行描述。

```python
while 虚拟机调用堆栈非空:
  if 当前栈帧对应的函数中存在可匹配的 `except` 或 `finally` 处理器:
    将解释器控制流转入该处理器执行
    break
  将当前栈帧从虚拟机调用堆栈中弹出
```
代码 2-16

那么，PVM 应当如何查找有效的异常处理器呢？自 CPython 3.11 起，引入了函数级异常表（exception table）的概念。也就是说，在异常发生后，由 PVM 依据触发异常的字节码指令地址，通过当前栈帧对应函数的异常表查询是否存在可匹配的处理器。这种方式使得正常执行路径几乎不需要为异常处理付出额外开销，同时也让 PVM 内部异常控制流与普通控制流之间的边界更加清晰。需要进一步指出的是，在真实的 PVM 系统中，异常表查询结果除了 Python 代码中异常处理器的入口地址外，通常还应包括解释器为清理异常现场而需要恢复的操作数栈深度等控制流信息。

与此相对应，虚拟机内部记录的异常状态也不能只保存“当前异常对象”本身。对 PVM 而言，异常状态通常还需要额外保存异常发生时的控制流信息，例如触发异常的指令地址等。

综上所述，对一个兼容 CPython 3.12 字节码模型的 PVM 而言，为了实现异常机制，至少要解决两大问题：其一，如何记录异常状态；其二，如何进行异常表查找与栈展开。在上述理论基础之上，本文所实现的 S.A.A.U.S.O VM 的异常系统正是围绕这两个问题展开设计的。

## 2.6 Python 中的模块机制

模块（module）机制是实际 Python 项目中组织代码和复用功能的重要方式。对用户而言，`import` 语句看起来很简单。但对 PVM 来说，模块导入相关的核心指令主要包括 `IMPORT_NAME` 和 `IMPORT_FROM` 等，而在这些字节码背后，完整的导入操作涉及名称解析、模块查找、代码加载、模块体执行、导入缓存以及父子模块绑定等多个环节，是一个典型的系统级功能。

具体来说，对于一次典型的导入操作，PVM 通常需要先判断目标模块是否已存在于缓存中；若不存在，则按照搜索路径（如 `sys.path`）查找模块文件或包入口，创建模块对象，并执行模块体初始化其命名空间。执行完成后，该模块会被登记到 `sys.modules`，供后续重复导入时复用。而对于包（package）来说，模块机制还需要额外处理父子层级关系、相对导入级别（level）以及 `from ... import ...` 的绑定语义。

## 2.7 本章小结

本章围绕执行模型、函数与栈帧、对象系统、异常机制和模块机制，概述了一个轻量级 PVM 后端需要解决的核心问题。概括而言，PVM 采用前后端分离的执行结构：前端负责生成字节码表示，后端负责字节码装载、调度、执行以及运行时语义支撑。本章的分析为第 3 章的总体设计与第 4 章的工程实现的提供了理论基础。

# 第3章 S.A.A.U.S.O VM 系统总体设计

第 2 章已经从理论层面说明了一个轻量级 Python 虚拟机后端所需要处理的几个关键问题，包括字节码执行、函数调用、对象模型、异常机制和模块导入等。本章在此基础上，进一步给出 S.A.A.U.S.O VM 的总体设计方案，并重点论述“系统整体如何划分”“各模块如何协作”以及“为什么采用这样的结构”。

## 3.1 设计目标与总体思路

S.A.A.U.S.O VM 的总体设计主要围绕以下几个目标展开。

首先，系统需要兼容 CPython 3.12 字节码模型中的核心执行路径，使常见的 Python 核心功能能够被正确解释执行。这里的“兼容”并不意味着逐项复刻完整的 CPython 实现，而是以教学和定制裁剪场景为目标，优先支持最有代表性的语言语义和运行时机制。

其次，系统需要保持良好的可嵌入性。也就是说，S.A.A.U.S.O VM 不仅要能够作为独立解释执行系统运行脚本，还应当能够通过清晰的对外接口嵌入到宿主 C++ 程序中，作为脚本引擎使用。基于这一考虑，系统设计应当预留专门的嵌入接口（Embedder API）层，并将其与 VM 内核实现进行隔离。

再次，系统需要具备较好的可读性和工程可维护性。对于一个轻量级 PVM 后端而言，真正困难的部分往往不在于单个子系统能否运行，而在于多个子系统之间能否形成稳定、清晰、可解释的协作方式。因此，系统设计应当尽量避免将 VM 中的能力集中耦合到解释器内部，而是尝试把对象系统、执行系统、运行时语义、模块系统和内存管理做成相对独立又彼此配合的层次结构。

基于上述目标，S.A.A.U.S.O VM 的总体设计思路可概括为：内核与嵌入接口解耦、子系统间低耦合高内聚、系统各层次间依赖关系明确。

## 3.2 系统总体分层架构

从整体结构上看，S.A.A.U.S.O VM 可以分为 VM 内核层与嵌入接口层两大部分。其中，PVM 的基础设施与核心能力均封装在 VM 内核当中，而嵌入接口则负责把这些能力以相对稳定、易用的形式暴露给宿主应用。

如果进一步按职责细分，自底层到高层，VM 内核可以进一步概括为如下几层：
- 工具基础层（Utils）：提供与虚拟机业务无关的通用工具与基础设施；
- 虚拟机堆（Heap）：提供虚拟机内部使用的堆空间及其内存分配接口，并负责承担垃圾回收等自动化内存管理任务；
- 句柄机制层（Handles）：提供对象句柄（object handle）机制，用于在垃圾回收场景下安全持有对象引用；
- 对象表示层（Objects）：定义 Python 对象的统一表示方式，以及类型元信息、对象布局和核心对象模型；
- 执行层（Execution）：封装 Python 语言运行时能力的实际实现，以及 Python 脚本的实际解释执行能力。

这种分层方式旨在服务于两个核心目标。其一，是降低模块耦合，避免解释器承担过多高层语义职责；其二，是为系统后续继续扩展语言特性、优化执行路径或加强嵌入接口打下更清晰的结构基础。

从依赖方向上看，系统整体遵循自底向上的单向依赖原则。这样的设计既能保持虚拟机内核的相对稳定、便于长期扩展，也有利于控制复杂度。

## 3.3 核心运行时容器设计

在 S.A.A.U.S.O VM 中，`Isolate` 是最核心的运行时容器。它既可以理解为一个独立的虚拟机实例，也可以理解为一个完整的 Python 运行时环境。系统中的虚拟机堆、工厂、解释器、模块系统等核心组件，以及内建函数环境、VM 异常状态和内建 Python 类型元信息，都会被集中挂载在某个 `Isolate` 之下。

引入 `Isolate` 作为整个 VM 系统的中心，有几个明显好处。第一，它把原本分散的运行时状态统一收敛到一个容器当中，减少了 VM 中全局状态数量繁多而带来的管理难度。第二，它天然适合作为嵌入接口中的最外层抽象：宿主程序只需要创建、进入和销毁一个 `Isolate`，即可拥有一个相对独立的“Python 世界”。第三，这种设计也为宿主程序提供了更高的灵活度；例如，宿主程序可以创建多个相互隔离的 Python 运行时环境，分别用于处理不同的业务逻辑。

围绕 `Isolate`，系统建立的若干核心组件包括：
- `Heap`：即虚拟机堆，负责 VM 堆内存空间管理、内存分配和垃圾回收；
- `Factory`：对象工厂，建立在虚拟机堆之上，负责提供统一的对象创建接口，屏蔽底层分配细节；
- `Interpreter`：即字节码解释器，负责字节码调度与函数调用；
- `ModuleManager`：即模块系统，负责模块导入和模块缓存逻辑；
- `ExceptionState`：负责保存当前待传播异常；
- `StringTable`：VM 中预置的字符串常量表
- 各类 `Klass`/`TypeObject`：各类内建 Python 类型的元信息。

在这种结构下，系统中的许多关键操作都不是直接跨模块互相调用，而是围绕 `Isolate` 这一上下文展开。例如，解释器执行时通过 `Isolate` 实例访问 builtins 和模块状态，运行时抛错时向具体的 `Isolate` 实例写入异常状态，嵌入接口创建对象时需要使用宿主方指定 `Isolate` 实例的对象工厂。这意味着在系统中执行关键操作时，都必须明确要在哪个具体的 `Isolate` 实例上执行操作。这使得系统的行为在工程上更容易追踪和分析。同时，这种设计也更便于统一管理各个 VM 核心组件的生命周期。

## 3.4 核心层的职责与设计要点

在前文 3.2 节中，本文已经给出了 S.A.A.U.S.O VM 的总体分层结构。在本节中，将从总体设计角度说明各核心层承担的核心职责，以及这些层的关键设计要点。

### 3.4.1 虚拟机堆

虚拟机堆负责承担对象分配、空间组织与垃圾回收等内存管理职责，是整个运行时对象体系得以成立的基础。对于 S.A.A.U.S.O VM 而言，堆层不仅要提供可用的分配能力，还要为后续对象系统、解释器、模块系统和异常状态管理提供统一的托管内存环境。

根据 David Ungar 在《Generation Scavenging: A non-disruptive high performance storage reclamation algorithm.》中提出的分代假说，编程语言 VM 中的对象具有不同的生命周期，其中大部分对象会在创建后快速死亡。因此，为了更方便地管理不同生命周期的对象，HotSpot、V8 等工业级 VM 普遍选择将虚拟机堆划分为多个堆空间。S.A.A.U.S.O VM 在总体设计上同样借鉴了这一思路。

在堆空间划分的总体设计上，S.A.A.U.S.O VM 中主要包括新生代空间（new space）、元数据空间（meta space）以及预留的老生代空间（old space）。其中，新生代空间负责承载大量短生命周期的普通运行时对象；元数据空间负责承载类型元信息、VM 内部字符串常量等持久存在的对象。VM 内部默认会在新生代空间中创建新对象，如果后续系统认为某个对象具有较长的寿命（例如系统发现某些对象在经历若干轮 GC 后仍然存活），那么它们就会被晋升（promotion）至老生代空间单独进行维护。

在垃圾回收策略的总体选择上，S.A.A.U.S.O VM 当前并没有采用类似于 CPython 中以引用计数为主路径的对象生命周期管理方式。原因在于，若以引用计数作为主路径，则还需要额外处理计数维护、循环引用检测等问题，这将会大幅增加本文系统的设计与实现难度。

基于上述考虑，S.A.A.U.S.O VM 在总体设计上选择了追踪式 GC （tracing GC）的技术路线。追踪式 GC 是一大类 GC 算法的统称，这类算法的基本思想是：先从一组显式可枚举的起点对象（即根集合，GC roots）出发，沿着对象之间的引用关系遍历整张对象图，并将遍历过程中可达（reachable）的对象视为存活对象，未被遍历到的对象则可视为垃圾。在这一框架下，垃圾回收的重点不再只是维护每个对象自身的局部计数，而是如何组织起点集合、如何正确遍历对象图。此外，如果 GC 过程中涉及到了对存活对象的移动，还需要解决在对象移动后同步修正持有方的引用指针的问题。

### 3.4.2 句柄机制层

在堆层之上，系统引入句柄机制层。

在 Python 程序运行过程中，除了 Python 程序层面对堆上对象形成的引用，VM 内部的 native C++ 代码中同样会持有对堆上对象的引用。因此 GC 算法需要明确知道这些引用的存在，才能避免被 native 持有的仍然存活的对象被误回收。其次，GC 算法在运行过程中，可能会移动存活对象在内存空间中的位置，因此 VM 中需要一种手段，能够保证 native C++ 中对堆上对象的引用（即指向堆上的指针）能够被正确且及时地更新。

为了解决这些问题，需要为 VM 引入对象句柄这一中间结构，用于代替裸指针（raw pointer）持有和代理访问堆上对象。由于对象句柄属于 VM 自身提供和管理的基础设施，因此它能够为后续 GC 机制统一感知并维护 native 代码所持有的对象引用提供结构基础。

需要说明的是，本文中句柄机制层的总体思路主要借鉴自 V8 的设计，即不让高层 native 代码直接长期持有裸对象地址，而是引入一个由 VM 统一管理的中间引用结构。不过，S.A.A.U.S.O VM 并未直接照搬其完整的工业级实现，而是围绕轻量级 PVM 的功能目标与实际需要，对其进行了简化与适配。在 4.2 中，会对本文系统中句柄机制的详细实现进行介绍。

### 3.4.3 对象表示层

对象表示层主要包括 VM 的对象系统。对象系统负责给出系统中 Python 对象的表示方式、内存布局、对象行为分派机制，并支撑 Python 作为面向对象语言应有的封装、继承与多态能力。

在总体设计上，S.A.A.U.S.O VM 采用“对象实例数据与类型行为分离”的思路。对象实例主要负责保存字段数据，而与类型相关的名称、继承关系、核心行为等，则交由独立的类型描述结构承担。这样做的目的在于能够把“对象持有什么数据”和“对象应如何响应操作”区分开来。同时 VM 中需要建立统一的对象核心行为分派机制，以避免解释器和运行时为每一种对象类型编写大量特判分支。

在这种设计下，VM 中的 Python 对象需要由两部分共同进行描述：一部分是该对象在内存中的实体（包括对象头与对象实体数据），另一部分是该对象关联的类型元信息。前者回答“这个对象当前保存了什么状态”，后者回答“这个对象是什么类型、能够参与哪些行为分派”。例如，字符串对象与列表对象在内存布局上显然不同，但解释器和运行时仍可以通过各自关联的类型描述结构，以统一方式发起属性访问、运算、迭代或调用等操作。与此同时，这套类型描述结构还需要承担部分类型关系维护和对象扫描支持（为了配合 GC 机制）职责，因此它既服务于语言语义，也服务于内存管理和执行调度。

需要说明的是，这种“对象实例数据与类型行为分离”的总体思路，灵感主要来源于 HotSpot JVM 中针对 Java 这种静态语言所设计的 Oop-Klass 模型。但本文系统并未照搬其具体实现，而是结合 Python 语言的动态性特征，和自定义内建类型的实际需要，对该思路进行了重新组织与落地实现。也正因如此，本文系统中的对象模型，既保留了工业级 VM 在对象表示层面的结构优势，又便于扩展新的内建类型并支撑 Python 语言的动态语义。在 4.3 中，将对本文系统的实际实现进行介绍。

### 3.4.4 执行层

执行层是最贴近 Python 程序实际运行过程的一层，负责提供真正的 Python 程序执行能力。这一层并非单一模块，而是由多个相互配合的子系统共同构成。

- 运行时语义子系统（Runtime）：负责承载跨对象、跨场景复用的高层语义逻辑；
- 内建能力子系统（Builtins）：负责提供内建函数、基础类型名和最小异常类型等语言运行环境；
- 模块子系统（Modules）：负责处理 Python 模块的导入操作，解析、查找、加载与缓存；
- 代码装载前端（Code）：负责把 Python 源码或 `.pyc` 输入转换为 VM 内部可执行表示；
- 字节码解释器（Interpreter）：Python 程序字节码序列真正的执行引擎，负责字节码分派、栈帧推进、函数调用和栈展开等核心执行任务。

将执行层细分多个子系统，有助于避免解释器独自承担全部执行语义职责，使整个系统在结构上更接近“多子系统协作完成程序执行”的高内聚低耦合的工程形态。

### 3.4.5 嵌入接口层

在 VM 内核之外，S.A.A.U.S.O VM 还提供独立的嵌入接口层，对外暴露 `Isolate`、`Context`、`Script`、`Value`、`Function`、`TryCatch` 等宿主可直接理解和使用的抽象，对内则通过桥接逻辑连接 VM 内核当中的执行层和对象系统。该层的核心目标，是在不泄漏内部实现细节的前提下，为宿主程序提供清晰、稳定且便于接入的脚本引擎接口。

## 3.5 本章小结

本章在第 2 章理论基础的前提下，进一步给出了 S.A.A.U.S.O VM 的系统总体设计。首先，本章中明确了系统的总体目标，即在保证核心 Python 字节码执行能力的同时，兼顾嵌入性、可读性与工程可维护性；随后，围绕这一目标给出了由工具基础层、虚拟机堆、句柄机制层、对象表示层、执行层和嵌入接口层构成的总体分层结构，并说明了各层的职责边界与依赖方向。

# 第4章 S.A.A.U.S.O VM 关键模块的实现

第 3 章已经给出了 S.A.A.U.S.O VM 的层次划分、核心运行时容器以及关键执行流程的总体设计。本章进一步说明这些设计在实际系统中的落地方式。

## 4.1 虚拟机堆的实现

对于一个以托管对象为核心的 PVM 而言，虚拟机堆是对象系统、解释器和运行时语义得以运作的前提。S.A.A.U.S.O VM 在这一部分并未直接复刻工业级 VM 的复杂内存系统，而是优先建立一个能够支撑当前功能闭环、同时便于后续扩展的轻量级内存托管与垃圾回收系统。

### 4.1.1 对象引用表示与 `Tagged<T>`

在讨论虚拟机堆之前，需要先介绍一下后文代码中频繁出现的 `Tagged<T>`。在 S.A.A.U.S.O VM 中，它用于统一表示运行时对象引用。之所以不直接使用 `PyObject*` 等裸指针，是因为系统采用了标记指针（tagged pointer）思想，复用同一套位表示同时容纳堆对象引用和整型立即数。

若直接把这种混合表示塞入裸指针，会破坏 C++ 编译器对指针对齐和值语义的要求，从而引入未定义行为。为此，系统借鉴 V8 等工业级 VM 的做法，引入 `Tagged<T>` 作为统一封装。后文出现的 `Tagged<PyObject>`、`Tagged<Klass>` 等，均应理解为运行时对象引用。

### 4.1.2 堆空间划分与实现

在 3.4.1 中，已经介绍了 S.A.A.U.S.O VM 中将虚拟机堆划分为多个堆空间的总体设计思路。

在实际实现中，S.A.A.U.S.O VM 的堆空间并不是一段简单连续内存，而是由若干页（page）构成。每个页都拥有固定页大小、页头元信息和线性分配状态，系统需要维护页链表与分配状态。

这种设计的价值在于：系统可通过页头记录所属空间和分配进度；可根据对象地址快速定位页面头部并判断空间类别；也便于后续实现空间的动态扩容或缩容。虽然现阶段系统尚未实现完整的老生代空间与分代式 GC，但页化空间已经为后续演进打下了结构基础。

代码 4-1 给出了 S.A.A.U.S.O VM 中页化空间的删节定义。

```cpp
class BasePage {
 private:
  friend class PagedSpace;

  PagedSpace* owner_;
  Address allocation_top_;
  BasePage* next_;
  BasePage* prev_;
};

class PagedSpace : public Space {
 protected:
  static void InitializePageHeader(BasePage* page,
                                   PagedSpace* owner,
                                   Address page_start,
                                   ...);
};

class NewSpace : public PagedSpace { ... };
class OldSpace : public PagedSpace { ... };
class MetaSpace : public PagedSpace { ... };
```
代码 4-1

从这段代码中可以看到，`BasePage` 负责保存页级元信息。页头中的 `owner_` 与 `allocation_top_` 分别用于说明该页属于哪个空间、当前页内已经分配到何处。而 `PagedSpace` 则负责页头初始化等公共能力，同时 `NewSpace`、`OldSpace` 和 `MetaSpace` 等，均是抽象基类 `PagedSpace` 的具体实现。

### 4.1.3 根集合与对象可达性

在 3.4.1 中，已经论述了 tracing GC 的总体思想，并引出了根集合与可达性的概念。在实现层面，S.A.A.U.S.O VM 的根集合并不单一，它既包括 `Isolate` 持有的关键运行时组件，也包括解释器调用栈、句柄机制、内建类型元信息、模块缓存和异常状态中的对象引用。

因此，垃圾回收不是堆模块的局部逻辑，而是整个 VM 的协作过程。只有把这些运行时状态都显式暴露给 GC，系统才能正确遍历全部存活对象。

代码 4-2 给出了 `Heap::IterateRoots()` 的删节实现。该函数把 VM 中若干关键运行时状态统一暴露给 GC 访问器，从而构成 tracing GC 执行扫描时的根集合。

```cpp
void Heap::IterateRoots(ObjectVisitor* v) {
  isolate_->Iterate(v);

  for (size_t i = 0; i < isolate_->klass_list().length(); ++i) {
    isolate_->klass_list().Get(i)->Iterate(v);
  }

  if (isolate_->handle_scope_implementer() != nullptr) {
    isolate_->handle_scope_implementer()->Iterate(v);
  }
  if (isolate_->interpreter() != nullptr) {
    isolate_->interpreter()->Iterate(v);
  }
  if (isolate_->module_manager() != nullptr) {
    isolate_->module_manager()->Iterate(v);
  }
  isolate_->exception_state()->Iterate(v);
}
```
代码 4-2

从这段代码可以看出，根集合在实现中被显式组织为一组运行时状态入口。若 `Heap::IterateRoots()` 漏掉某个入口，GC 就可能错误回收仍然存活的对象。因此，对 tracing GC 而言，根的组织与 GC 算法同样关键。句柄机制作为重要扫描根，会在 4.2 中进一步展开。

### 4.1.4 具体的垃圾回收算法与实现

在现阶段的 S.A.A.U.S.O VM 中，主要实装的是 Cheney 算法，这是追踪式 GC 算法的一种。它由 C. J. Cheney 在论文《A nonrecursive list compacting algorithm》中提出。在 HotSpot、V8 等工业级 VM 的实现中，该算法主要被用作针对新生代空间的 GC 算法，亦被称为 Scavenge 算法；在 S.A.A.U.S.O VM 的代码实现中，同样沿用了这一称呼。

在介绍 Cheney 算法之前，这里首先需要说明现阶段选择该算法的原因。在工业级 VM 中常用的 tracing GC 算法还包括标记-清除（mark-sweep）和标记-整理（mark-compact）算法。但本文系统并未将它们作为第一阶段方案。前者虽然概念直接，但需要处理空闲块管理与内存碎片问题；后者虽然能进一步改善碎片，却会引入复杂的对象位置调整与引用更新流程。相比之下，Cheney 算法的实现相对简单，且不会引入外部内存碎片，更适合当前阶段 VM 系统先形成可用的基础垃圾回收能力，再逐步演进的目标。

Cheney 算法的基本思想是：
（1）将堆空间进一步划分为 Eden 区与 Survivor 区，新分配对象首先进入 Eden 区。
（2）当 GC 启动时，系统从根集合出发找到仍然存活的对象，将它们复制到 Survivor 区，并同步修正根集合对它们的引用。
（3）扫描这些被复制到 Survivor 区的存活对象，进一步找出它们所持有的、仍在 Eden 区的对象，再将这些存活对象复制到 Survivor 区，并修正前者对它们的引用。
（4）如此循环往复，直至所有存活对象都被复制至 Survivor 区。
（5）最后交换半空间的角色，将原先的 Survivor 区作为 Eden 区、原先的 Eden 区作为 Survivor 区，以便下一轮 GC 时可以重复上述操作。

代码 4-3 给出了 `ScavengerCollector` 与 `ScavengeVisitor` 的删节实现。前者负责组织回收流程，后者负责对具体槽位中的对象执行复制、转发与引用更新。

```cpp
void ScavengerCollector::CollectGarbage() {
  ScavengeVisitor visitor(heap_);
  heap_->IterateRoots(&visitor);

  while (scan_page != nullptr) {
    Tagged<PyObject> object(scan_ptr);
    size_t instance_size = PyObject::GetInstanceSize(object);
    PyObject::Iterate(object, &visitor);
    scan_ptr += instance_size;
  }

  heap_->new_space()->Flip();
}

void ScavengeVisitor::EvacuateObject(Tagged<PyObject>* slot_ptr) {
  if (mark_word.IsForwardingAddress()) {
    *slot_ptr = mark_word.ToForwardingAddress();
    return;
  }

  size_t size = PyObject::GetInstanceSize(object);
  Address target_addr = AllocateInSurvivorSpace(size);

  std::memcpy(reinterpret_cast<void*>(target_addr),
              reinterpret_cast<void*>(object.ptr()), size);
  PyObject::SetMapWordForwarded(object, Tagged<PyObject>(target_addr));
  *slot_ptr = Tagged<PyObject>(target_addr);
}
```
代码 4-3

这段代码揭示了当前 GC 逻辑的几个关键工程特征。首先，回收并不是从堆中“盲目扫描全部对象”开始，而是从根集合出发，把所有可达对象逐步复制到 Survivor 区。其次，复制完成后，Eden 区中原对象位置会被写入该对象被复制至的新内存地址（被称为转发地址，forwarding address）；后续若再遇到指向该对象在 Eden 区中旧位置的引用，系统不需要重复复制，只需读取转发地址并让外部引用指向它即可。再次，`ScavengerCollector` 并未采用递归式的 DFS 遍历，而是以 Survivor 空间中的已复制对象作为一个隐式的 BFS 队列，按线性扫描方式不断推进，这种做法既避免了深递归，又无需开辟额外的内存空间。

### 4.1.5 工厂函数与对象的初始化

在建立虚拟机堆之后，就可以在它上面分配内存并创建对象了。在 S.A.A.U.S.O VM 中，引入了统一的工厂函数（factory function）用于在堆上创建对象并完成初始化。一般情况下，VM 的上层业务逻辑只需要直接调用这些工厂函数即可。

代码 4-4 展示了本文系统中部分常用的工厂函数。

```cpp
class Factory {
 public:
  Handle<PyDict> NewPyDict(int64_t init_capacity);
  Handle<PyList> NewPyList(int64_t init_capacity);
  Handle<PyString> NewString(const char* source, int64_t str_length, bool in_meta_space);
  Handle<PyFunction> NewPyFunctionWithCodeObject(Handle<PyCodeObject> code_object);
  // 后略...
};
```
代码 4-4

首先需要指出的是，这段代码中出现的 `Handle<T>` 即 3.4.2 中提到的对象句柄，用于在 C++ 代码中安全持有堆 VM 对象的引用；其具体机制将在 4.2 节中进一步展开。此处可先将其理解为工厂函数返回新分配对象时所使用的统一引用包装形式。

在本文系统中引入工厂函数，主要是为了收敛对象分配与初始化逻辑、封装堆分配细节，并避免上层代码因初始化顺序不当而出错。这在可能因分配内存而触发 GC 的系统中尤其重要：若对象的指针字段尚未初始化完成便触发 GC，扫描过程就可能访问到无效引用（即野指针）。为了更具体地说明，代码 4-5 给出了 `Factory::NewDict()` 的删节实现。

```cpp
Handle<PyDict> Factory::NewDict(int64_t init_capacity) {
  ...
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace),
                        isolate_);
  {
    ...
    object->set_occupied(0);
    object->set_data(Tagged<FixedArray>::null());
    PyObject::SetProperties(*object, Tagged<PyDict>::null());
  }

  Handle<FixedArray> data = NewFixedArray(init_capacity * 2);
  object->set_data(data);

  return object;
}
```
代码 4-5

从这段代码可以看到，字典对象被分配后并不会立刻申请内部数据结构，而是先完成最基本的字段初始化，将指针字段预填充为 null。这样，即使后续因再次分配而触发 GC，也不会因为未初始化指针而出错。因此，本文系统中的工厂函数均按先完成对象基础字段初始化，再执行后续依赖堆分配的步骤进行实现。

## 4.2 句柄机制实现

在 3.4.2 中，已经从总体设计角度论述了引入句柄机制的背景与动机。本节进一步说明句柄在 S.A.A.U.S.O VM 中的实际使用方式及其内部实现。

### 4.2.1 VM 代码中的句柄使用方式

在介绍句柄机制的内部实现之前，先说明它在 VM 日常开发中的典型用法。总体上，开发者很少在高层逻辑中直接使用 4.1.1 中介绍的 `Tagged<T>` 传递堆对象引用，而是优先使用 `Handle<T>`；当某段逻辑可能临时创建一批新的对象引用时，通常还需要建立相应的句柄作用域，即 `HandleScope` 或 `EscapableHandleScope`。这样，native C++ 代码中的临时对象引用就能够被统一收敛到句柄槽位中，进而被 GC 感知和更新，而不会演变为难以追踪的裸指针。

### 4.2.2 对象句柄与槽位模型

S.A.A.U.S.O VM 中句柄机制层的核心组件为 `HandleScopeImplementer`。该组件会负责管理一批内存块（block）。每个内存块会被视作一个提供若干 `Address` 槽位（slot）的定长 C 数组。在本文系统中，`Address` 用于代表指向堆上对象的内存地址。

从实现本质上看，`Handle<T>` 并没有直接保存对象地址，而是保存了某个槽位的位置。这些槽位就来自 `HandleScopeImplementer` 管理的内存块。换言之，槽位中放置了实际指向堆上对象的地址，而 `Handle<T>` 自身则保存指向该槽位的指针，即 `Address* location_`。因此，当 VM 通过 Handle 访问对象时，逻辑上存在两次解引用：第一次对 `location_` 解引用，取出槽位中保存的对象地址；第二次再根据该对象地址访问真实对象内容。

在实际使用时，除直接拷贝 `Handle<T>` 外，开发者往往先拿到原始对象的 `Tagged<T>`，再向 `HandleScopeImplementer` 申请槽位并写入地址。也就是说，对象句柄的构造过程本质上就是“申请槽位并把对象地址写进去”。

代码 4-7 给出了上述机制的删节实现。可以看到，`Handle<T>` 负责保存并解引用槽位，而 `HandleScopeImplementer::CreateHandle()` 则负责完成“从原始对象地址申请槽位并写入对象地址”的过程。同时，`HandleScopeImplementer` 负责维护整片内存块的分配状态。

```cpp
using Address = uintptr_t;

template <typename T>
class Handle {
 public:
  Handle(Tagged<T> tagged, Isolate* isolate) {
    if (object != kNullAddress) {
      location_ = HandleScopeImplementer::CreateHandle(isolate, object.ptr());
    }
  }
  ...
  constexpr T* operator->() const { return Tagged<T>(*location()).operator->(); }

 private:
  Address* location_{nullptr};
};

class HandleScopeImplementer {
 public:
  static constexpr int kHandleBlockSize = 512;
  static Address* CreateHandle(Isolate* isolate, Address ptr);

 private:
  Vector<Address*> blocks_;
};
```
代码 4-7

槽位模型是解决 3.4.2 中所提出问题的关键。在 4.2.4 中会进一步说明它所起到的作用。

### 4.2.3 HandleScope 的作用域回退与长期句柄

在 4.2.2 中提到，创建 `Handle<T>` 时需要向 `HandleScopeImplementer` 申请槽位。那么反过来，本文系统中也需要一种机制，能够在对象句柄失效时及时释放其占用的槽位。这样，才能建立对象句柄槽位的生命周期闭环。

在局部执行场景中，S.A.A.U.S.O VM 通过 `HandleScope` 管理 Handle 槽位的生命周期。如代码 4-8 给出的删节代码所示，`HandleScope` 在创建时会保存当前 `HandleScopeImplementer` 的分配状态，在退出析构时再把该状态恢复到进入该作用域之前的位置。于是，该作用域内后续申请出去的局部 slot 就可以被整体视为“无效并可复用”。这正是 `HandleScope` 能够批量释放局部 Handle 所申请槽位的本质原因。

```cpp
HandleScope::HandleScope(Isolate* isolate) {
  auto* impl = isolate->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();
  previous_ = *handle_scope_state;
}

void HandleScope::~HandleScope() {
  auto* impl = isolate_->handle_scope_implementer();
  auto* handle_scope_state = impl->handle_scope_state();
  impl->set_handle_scope_state(&previous_);
}
```
代码 4-8

这种设计的好处在于，`HandleScope` 对槽位的释放是批量完成的，比逐个 `Handle` 释放更适合 VM 这类对性能敏感的系统。

### 4.2.4 与 GC 机制的协同

在建立了对象句柄的槽位模型，并明确槽位的生命周期管理之后，下一步就是实现句柄机制与 GC 机制的协同。

在 4.1.3 中已经提到，`HandleScopeImplementer` 作为根集合的一部分，会在 GC 发生时被 GC 访问器扫描。具体到实现，GC 访问器会逐个扫描 `HandleScopeImplementer` 所管理内存块当中已经被分配出去的有效槽位。扫描过程中，访问器会逐个读取槽位中的对象地址，并在对象位置被调整后将槽位中的内容直接更新为新地址。这样一来，一方面 GC 机制就成功找到了所有在 native C++ 代码中被对象句柄持有的存活对象；另一方面，只要后续代码仍通过 Handle 访问对象，就总能经由对应槽位间接访问到移动后的最新地址，而非已经失效的旧地址。

代码 4-9 给出了这一过程的删节实现。

```cpp
void HandleScopeImplementer::Iterate(ObjectVisitor* v) {
  if (blocks_.length() > 1) {
    for (size_t i = 0; i < blocks_.length() - 1; ++i) {
      Address* block_begin = blocks_.Get(i);
      Address* block_end = block_begin + kHandleBlockSize;
      v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                       reinterpret_cast<Tagged<PyObject>*>(block_end));
    }
  }

  if (!blocks_.IsEmpty()) {
    Address* block_begin = blocks_.GetBack();
    Address* block_end = handle_scope_state_.next;
    v->VisitPointers(reinterpret_cast<Tagged<PyObject>*>(block_begin),
                     reinterpret_cast<Tagged<PyObject>*>(block_end));
  }
}
```
代码 4-9

在现阶段的实现中，这里的 GC 访问器即为 4.1.4 中提及的 `ScavengeVisitor`。它会把槽位所指对象复制到 Survivor 空间，并把槽位内容更新为新地址。

```cpp
void ScavengeVisitor::VisitPointers(Tagged<PyObject>* start,
                                    Tagged<PyObject>* end) {
  for (Tagged<PyObject>* p = start; p < end; ++p) {
    Tagged<PyObject> object = *p;
    if (!CanEvacuate(object)) {
      continue;
    }
    EvacuateObject(p);
  }
}
```
代码 4-10

概括而言，S.A.A.U.S.O VM 把“维护 native 代码持有的堆对象引用”转化为“维护一组已知槽位”，从而解决了 3.4.2 中提出的问题。

## 4.3 对象系统实现

在 3.4.3 中，本文已经从总体设计角度说明了对象表示层的职责与思路。本节进一步讨论这一思路在 S.A.A.U.S.O VM 中的具体落地。

### 4.3.1 `PyObject` 与 `Klass` 的分工

在 3.4.3 中，已经提出了“对象实例数据与类型行为分离”的对象模型设计总思路。在具体实现上，S.A.A.U.S.O VM 中所有 Python 对象的实体都统一纳入 `PyObject` 体系；对象对应的类型元信息则由 `Klass` 体系统一描述。`PyObject` 负责描述对象实体的布局与字段组织，`Klass` 负责描述对应 Python 类型的行为、继承关系等元信息。在 S.A.A.U.S.O VM 中，系统会为每一种内建类型，以及用户在 Python 程序中自定义的类创建唯一的一个对应 `Klass` 实体。

在进一步介绍 `PyObject` 和 `Klass` 各自承载的数据之前，首先需要回答一个问题：`PyObject` 实体是如何与 `Klass` 建立关联的呢？

在本文系统中，`PyObject` 在对象起始位置预留 `MarkWord` 字段，用于在正常运行时保存对象所属 `Klass` 的地址。借助这一字段，`PyObject` 实体就建立起了与 `Klass` 元信息的关联。

代码 4-11 给出了 `PyObject` 与 `MarkWord` 相关实现的删节代码。

```cpp
class PyObject : public Object {
 public:
  static Tagged<Klass> ResolveObjectKlass(Tagged<PyObject> object);
  static void SetKlass(Tagged<PyObject> object, Tagged<Klass> klass);

 private:
  MarkWord mark_word_;
};

Tagged<Klass> PyObject::ResolveObjectKlass(Tagged<PyObject> object) {
  return object->mark_word_.ToKlass();
}

void PyObject::SetKlass(Tagged<PyObject> object, Tagged<Klass> klass) {
  object->mark_word_ = MarkWord::FromKlass(klass);
}
```
代码 4-11

需要指出的是，`Klass` 实体仅供 VM 内部使用，Python 程序只能直接观察到类型对象。为打通二者，本文系统在 `Klass` 与 `PyTypeObject` 之间建立了双向关联：运行时可由类型对象快速定位到 `Klass` 实体，反之亦然。相关删节代码如 4-12 所示。

```cpp
class Klass {
  ...
  Tagged<PyObject> type_object_;
};

class PyTypeObject : public PyObject {
  ...
  Tagged<Klass> own_klass_;
};
```
代码 4-12

### 4.3.2 实例属性、类属性与方法绑定

S.A.A.U.S.O VM 并没有把 Python 对象实例的所有属性都简单地堆入同一个容器中，而是将实例属性与类属性分开组织，这与 2.4.3 中关于“字典化封装”问题的分析结论是一致的。在实际系统中，实例对象通过 `PyObject::properties_` 持有自身的属性字典，类型对象对应的 `Klass` 则通过 `klass_properties_` 持有类属性字典。这样一来，“对象当前持有的具体状态”和“类型为所有实例共享的属性与方法”就在结构上被分开表示了。

代码 4-13 给出了实例属性字典、类属性字典的删节定义；并以创建类字典对象为例，给出了实例属性字典分配策略的删节实现。

```cpp
class PyObject {
 public:
  Tagged<PyObject> properties_;
};

class Klass {
 public:
  Tagged<PyObject> klass_properties_;
};

Handle<PyDict> Factory::NewDictLike(Tagged<Klass> klass_self,
                                    int64_t init_capacity) {
  EscapableHandleScope scope(isolate_);
  Handle<PyDict> object(Allocate<PyDict>(Heap::AllocationSpace::kNewSpace),
                        isolate_);
  ...
  return scope.Escape(object);
}
```
代码 4-13

下面进一步在代码 4-14 和 4-15 中分别给出 `PyObjectKlass::Generic_GetAttr()` 以及它所依赖的 `Runtime_LookupPropertyInInstanceTypeMro()` 的删节实现，用于说明 S.A.A.U.S.O VM 中默认的 Python 对象属性查找与方法绑定逻辑。

```cpp
bool PyObjectKlass::Generic_GetAttr(Isolate* isolate,
                                    Handle<PyObject> self,
                                    Handle<PyObject> prop_name,
                                    ...) {
  ...
  Handle<PyObject> result;
  Handle<PyDict> properties = PyObject::GetProperties(self);
  if (!properties.is_null()) {
    result = PyDict::Get(properties, prop_name);
    if (!result.is_null()) {
      goto found;
    }
  }

  result = Runtime_LookupPropertyInInstanceTypeMro(self, prop_name);
  if (!result.is_null()) {
    if (IsPyFunction(result)) {
      result = isolate->factory()->NewMethodObject(result, self);
    }
    goto found;
  }

  Handle<PyObject> getattr_func = Runtime_LookupPropertyInInstanceTypeMro(
                                      self, "__getattr__");
  if (!getattr_func.is_null()) {
    result = Execution::Call(isolate, getattr_func, ...);
    goto found;
  }
  ...
  Runtime_ThrowErrorf(isolate, ExceptionType::kAttributeError,
                      "'%s' object has no attribute '%s'\n", ...);
  ...
}
```
代码 4-14

```cpp
Handle<PyObject> Runtime_LookupPropertyInInstanceTypeMro(
    Handle<PyObject> instance,
    Handle<PyObject> prop_name) {
  ...
  Tagged<Klass> object_klass = PyObject::ResolveObjectKlass(instance, isolate);
  Handle<PyList> mro_of_object = object_klass->mro(isolate);
  for (auto i = 0; i < mro_of_object->length(); ++i) {
    ...
    Handle<PyObject> result = PyDict::Get(current_klass_properties, prop_name);
    if (!result.is_null()) {
      return result;
    }
  }
  ...
}
```
代码 4-15

从这部分实现可以看到，S.A.A.U.S.O VM 中属性查找与方法绑定的主流程与 2.4.3 和 2.4.4 的理论分析是一致的：先查实例字典，再沿 MRO 查类字典；若命中函数对象，则构造绑定方法；若仍未找到，则尝试 `__getattr__`，最后抛出 `AttributeError` 异常。

### 4.3.3 基于 `Klass/KlassVtable` 的对象核心行为分派机制

根据 2.4.5 中所做的理论分析，在通过字典查找与方法绑定维持语言层动态性的同时，为保证对象核心行为调用的效率，VM 内部仍然需要为若干高频核心行为提供更加稳定和高效的入口。而在实现阶段，S.A.A.U.S.O VM 又对对象"核心行为"的概念进行了进一步泛化，让它不仅仅包括 `__add__`、`__call__`、`__iter__`、`__len__` 等与 Python 语言语义直接相关的操作，也包括实例大小计算、对象扫描等 PVM 需要使用的内部能力。

在本文系统中，基于 `Klass` 又进一步引入虚函数表机制 `KlassVtable`，用于统一组织对象核心行为的分派。代码 4-17 给出了其删节定义。

```cpp
class Klass : public Object {
 public:
  Handle<PyDict> klass_properties(Isolate* isolate);
  Handle<PyList> supers(Isolate* isolate);
  Handle<PyList> mro(Isolate* isolate);
  const KlassVtable& vtable() const { return vtable_; }

 protected:
  KlassVtable vtable_;
  Tagged<PyObject> klass_properties_;
};

#define KLASS_VTABLE_SLOT_EXPOSED(V) \
  V(..., add, "__add__", Add)        \
  V(..., getattr, "__getattr__", GetAttr) \
  V(..., call, "__call__", Call)     \
  V(..., len, "__len__", Len)        \
  V(..., new_instance, "__new__", NewInstance) \
  V(..., init_instance, "__init__", InitInstance) \
  ...

class KlassVtable {
 public:
  Maybe<void> Initialize(Isolate* isolate, Tagged<Klass> klass);

#define DEFINE_VTABLE_SLOT(signature, field_name, ...) \
  signature field_name##_{nullptr};
  KLASS_VTABLE_SLOT_LIST(DEFINE_VTABLE_SLOT)
#undef DEFINE_VTABLE_SLOT

 private:
  Maybe<void> UpdateOverrideSlots(Isolate* isolate, Tagged<Klass> klass);
};
```

通过提供虚函数表，系统在调用对象的核心操作时，就可以直接沿着 `PyObject -> Klass -> KlassVtable` 这条路径直接找到并调用相应的内部 C++ 实现，而无需再执行开销昂贵的属性查找与方法绑定操作。例如，代码 4-18 中给出了本文系统中暴露给运行时、解释器等上层组件使用的 Python 对象加法操作接口的删节逻辑，其中就封装了这一条基于虚函数表的调用路径。

```cpp
Handle<PyObject> PyObject::Add(...) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  return klass->vtable().add_(isolate, self, other);
}
```
代码 4-18

与此同时，Python 仍保留了高度动态的行为覆写能力。为兼顾语言层动态性与内部执行效率，现阶段系统采用“优先走虚函数表，必要时退化为字典查找”的两层多态方案：若初始化类型虚函数表时发现用户重载了某个魔法方法，就把对应槽位改写为桥接函数，再走常规的属性查找与方法调用慢路径。

代码 4-19 给出了这一策略在实际系统中的删节实现。

```cpp
void KlassVtable::UpdateOverrideSlots(Isolate* isolate,
                                      Tagged<Klass> klass) {
  Handle<PyDict> klass_properties = klass->klass_properties(isolate);

  bool has_magic_method = PyDict::Get(klass_properties, "__add__", ...);
  if (has_magic_method) {
    add_ = &KlassVtableTrampolines::Add;
  }

  其他魔法函数同理...
}

Handle<PyObject> KlassVtableTrampolines::Add(...) {
  ...
  Handle<PyObject> result = 
      Runtime_InvokeMagicOperationMethod(isolate, self, args, kwargs, "__add__");
  return result;
}
```
代码 4-19

在这段代码中，`KlassVtable::UpdateOverrideSlots()` 会在初始化用户自定义类的虚函数表时被调用。若发现用户重载了某个魔法函数，就把相应槽位改写为 `KlassVtableTrampolines` 中的桥接函数。例如，若用户重写了 `__add__`，系统在处理该类型实例的加法操作时就会进入 `KlassVtableTrampolines::Add()`，再通过常规的属性查找和函数调用路径执行该方法。

### 4.3.4 内建类型的实现原理及初始化

在 2.4.2 中已经指出，一个最小可用的 PVM 必须先提供若干基础内建类型。从实现角度看，这些类型并不是由用户代码在运行时临时创建的，而是由 VM 在 C++ 侧预先定义好的对象类型。因此，PVM 既要为它们提供各自的实例布局，也要准备对应的类型元信息与核心行为实现。

在 S.A.A.U.S.O VM 中，这种“预先定义”主要体现在两层：对象实体层由 `PyObject` 子类承载具体字段布局，类型元信息层由对应的 `Klass` 子类承载名称、继承关系、实例特征和核心行为入口。需要指出的是，并非所有内建值都一定以普通堆对象出现；例如当前系统中的整数采用标记指针表示，但仍通过 `PySmiKlass` 被统一纳入对象系统。

以 Python 中的 `list` 类型为例，代码 4-20 给出了本文系统内部对应的 C++ 对象类 `PyList` 的删节定义。

```cpp
class PyList : public PyObject {
 private:
  int64_t length_{0};
  Tagged<FixedArray> array_;
};
```
代码 4-20

从这段代码可以看到，`PyList` 在 `PyObject` 基础上增加了记录长度和指向元素存储区的字段。

对单个内建类型而言，其初始化过程与前文介绍的统一对象模型是衔接的：在 VM 启动阶段，系统会创建对应 `Klass`，填写元信息，计算 MRO，初始化虚函数表，安装内建方法，并建立类型对象与 `Klass` 的双向绑定。

以 `list` 类型为例，代码 4-21 给出了其初始化过程。

```cpp
void PyListKlass::PreInitialize(Isolate* isolate) {
  填写基础元信息，代码略...

  // 将内建类型的特有核心行为入口直接写进虚函数表的槽位
  vtable_.Clear();
  vtable_.new_instance_ = &Virtual_NewInstance;
  vtable_.init_instance_ = &Virtual_InitInstance;
  vtable_.len_ = &Virtual_Len;
  ...
}

void PyListKlass::Initialize(Isolate* isolate) {
  // 创建对应的类型对象，并建立双方的绑定关系
  CreateAndBindToPyTypeObject(isolate);

  // 初始化类型字典
  auto klass_properties = PyDict::New(isolate);
  set_klass_properties(klass_properties);

  // 将 object 作为 list 的父类，并计算生成 mro 序列
  AddSuper(PyObjectKlass::GetInstance(isolate), isolate);
  OrderSupers(isolate);

  // 根据继承关系填充虚函数表中剩余的空缺槽位
  vtable_.Initialize(isolate, Tagged<Klass>(this));

  // 安装内建方法到类型字典
  PyListBuiltinMethods::Install(isolate, klass_properties);

  // 设置类名
  set_name(PyString::New(isolate, "list"));
}
```
代码 4-21

需特别说明的是，为避免自举阶段的循环依赖，现阶段系统采用分阶段初始化：先执行各内建类型中无需依赖其他类型的 `PreInitialize()`，使系统达到最小可用；再执行依赖关系更完整的 `Initialize()`，补齐类型的 Python 语义。

### 4.3.5 用户自定义类的创建过程

对本文系统来说，创建用户自定义类，本质上是同时完成 Python 语义层可见的类型对象建立，以及 `Klass` 中元信息、继承关系和虚函数表的初始化。其中 `Klass` 的初始化尤为重要，因为它是 4.3.2 中属性查找与 4.3.3 中核心行为分派得以正确工作的基础。代码 4-20 给出了这一过程的删节实现。

```cpp
Handle<PyTypeObject> Runtime_CreatePythonClass(
    Isolate* isolate,
    Handle<PyString> class_name,
    Handle<PyDict> class_properties,
    Handle<PyList> supers) {
  EscapableHandleScope scope(isolate);
  
  Handle<PyTypeObject> type_object = isolate->factory()->NewPyTypeObject();
  Tagged<Klass> klass = isolate->factory()->NewPythonKlass();

  klass->set_klass_properties(class_properties);
  klass->set_name(class_name);

  if (supers->IsEmpty()) {
    PyList::Append(supers, base_object_klass, isolate);
  }
  klass->set_supers(supers);

  type_object->BindWithKlass(klass, isolate);
  
  klass->OrderSupers(isolate);
  klass->InitializeVtable(isolate);

  return scope.Escape(type_object);
}
```
代码 4-20

在这段代码中，系统首先创建新的类型对象和 `Klass` 并完成绑定；随后把类名、类属性字典以及基类列表写入 `Klass`；若用户没有显式声明父类，则自动补入 `object` 作为默认基类。

接下来，系统会进一步调用 `Klass::OrderSupers()`，以计算类型的 MRO 序列，其删节实现如代码 4-21 所示。

```cpp
void Klass::OrderSupers(Isolate* isolate) {
  Handle<PyList> all = PyList::New(isolate, supers->length());
  for (auto i = 0; i < supers->length(); ++i) {
    auto super = supers->Get(i, isolate));
    PyList::Append(all, C3Impl_Linear(isolate, super), isolate);
  }
  Handle<PyList> mro_result = C3Impl_Merge(isolate, all);
  PyList::Insert(mro_result, 0, type_object(isolate), isolate);

  mro_ = *mro_result;
}
```
代码 4-21

在这段代码中，会对基类列表执行 C3 线性化算法，得到该类型最终的 MRO 序列。计算完成后，系统会先把当前类型自身插入到序列开头，再将结果存入 `Klass` 内部的 `mro_` 字段。只有完成这一系列步骤，4.3.2 中介绍的 `Runtime_LookupPropertyInInstanceTypeMro()` 函数才能够从 `Klass` 实体中取得 MRO 序列并进行遍历。

在确定 MRO 序列后，系统还必须初始化新建类型的 `KlassVtable`。从代码 4-22 可以看到，`KlassVtable::Initialize()` 首先会沿着 MRO 序列复制祖先类中可继承核心行为的虚函数槽位；随后再调用 4.3.3 中已经分析过的 `UpdateOverrideSlots()`，检查当前类是否重载了 `__add__` 等魔法方法，并在必要时把对应槽位改写为桥接函数。这样，用户自定义类一方面能够继承父类已有的核心行为实现，另一方面也能够在保持 Python 动态语义的前提下，对这些行为进行覆写。

```cpp
void Klass::InitializeVtable(Isolate* isolate) {
  return vtable_.Initialize(isolate, this);
}

void KlassVtable::Initialize(Isolate* isolate, Tagged<Klass> klass) {
  InitializeFromSupers(isolate, klass);
  UpdateOverrideSlots(isolate, klass);
}

void KlassVtable::InitializeFromSupers(Isolate* isolate, Tagged<Klass> klass) {
  Handle<PyList> mro = klass->mro(isolate);
  for (int64_t i = 1; i < mro->length(); ++i) {
    Handle<PyTypeObject> super = mro->Get(i, isolate);
    CopyInheritedSlotsFromSuper(super->own_klass());
  }
}
```
代码 4-22

## 4.4 执行层基础设施的实现

在完成堆、句柄机制和对象系统之后，S.A.A.U.S.O VM 才具备“让 Python 程序运行起来”的基础。但真正推动字节码逐条执行的解释器并不是执行层的全部；在此之前，系统还必须完成运行时入口收口、内建环境装配、模块导入以及代码装载等准备工作。本节介绍这些基础设施，字节码解释器则放到下一节集中讨论。

### 4.4.1 执行入口的统一组织

S.A.A.U.S.O VM 并没有把脚本执行向 VM 上层直接暴露为解释器底层接口，而是通过统一执行入口对主脚本执行、模块执行和函数调用进行收口。这样既避免其他模块直接耦合解释器内部实现，也便于在进入解释器前统一准备 `locals`、`globals` 等上下文信息。

与此同时，一部分不适合直接写入解释器主循环的高层语义逻辑，例如类属性查找和用户自定义类型创建等，也统一由 `Runtime_` 接口承载。这样，解释器可以更专注于字节码调度、栈帧切换和异常控制流等底层执行逻辑，而执行层基础设施则负责组织这些公共入口与运行时辅助能力。

### 4.4.2 内建能力实现

如 2.3.4 所述，一个 PVM 即使已经拥有对象系统和解释器，如果没有基本的内建运行环境，仍然无法真正“可用”。因此，S.A.A.U.S.O VM 在执行层中专门组织了内建能力子系统，用于承载和提供内建能力的具体实现。

在现阶段的 S.A.A.U.S.O VM 中，`Isolate` 容器中持有一个 `builtins_` 字典，当解释器需要调用内建能力时就会访问这个字典，凭目标内建函数的名称进行查询。全体内建能力的 C++ 实现，会在 `Isolate` 容器初始化时被包装成 Python 函数对象并注入进 `builtins_` 字典。

与此同时，正如 2.3.4 中所讨论的，对于用户程序而言，`print`、`len` 等内建函数与普通 Python 函数都表现为“可调用对象”。这意味着在 PVM 内部，需要区分调用目标应该走普通 Python 函数的调用路径，还是直接调用 native C++ 逻辑。这个问题会在后文 4.5.1 中进一步讨论。

### 4.4.3 模块系统实现

正如 2.6 所讨论的，模块导入虽然由解释器通过导入相关字节码触发，但其背后是一套完整的系统逻辑。因此，S.A.A.U.S.O VM 将模块系统设计为执行层中的相对独立子系统，而不是把名称解析、模块定位、模块加载和缓存维护等逻辑耦合进解释器主循环。

其中，模块管理器 `ModuleManager` 持有并管理模块缓存 `modules_` 和导入路径 `path_`（对应于 Python 语言层面的 `sys.modules` 与 `sys.path`），并向解释器提供导入模块的功能入口 `ModuleManager::ImportModule()`。此外，模块子系统中的各个功能组件均由 `ModuleManager` 负责持有。

代码 4-23 给出了 `ModuleManager` 的删节实现。
```cpp
class ModuleManager {
 public:
  Handle<PyModule> ImportModule(...) {
    return importer_->ImportModule(...);
  }

 private:
  friend class ModuleImporter;
  ...
  std::unique_ptr<ModuleFinder> finder_;  // 在平台目录中查找 Python 模块文件
  std::unique_ptr<ModuleLoader> loader_;  // 加载暂未被缓存的 Python 模块
  std::unique_ptr<ModuleImporter> importer_; // 模块导入逻辑的调度枢纽
  std::unique_ptr<ModuleNameResolver> name_resolver_;  // 解析导入模块名

  Tagged<PyDict> modules_;
  Tagged<PyList> path_;
};
```
代码 4-23

从这段代码可知，当解释器调用 `ModuleManager::ImportModule()` 接口后，实则会进一步调用 `ModuleImporter::ImportModule()`。

`ModuleImporter` 是模块子系统中核心导入逻辑的调度枢纽。在 `ModuleImporter` 中，系统首先调用 `ModuleNameResolver` 组件解析导入模块名中的相对导入符号，再进一步调用 `ModuleImporter::ImportModuleImpl()` 根据完整的模块名分段进行导入。例如，如果用户想要导入的完整模块名为 `a.b.c`，则首先需要导入包 a，再导入子包 b，最后导入子包 b 中的模块 c。由于在 Python 中包被视作一种特殊的模块对象，因此这部分逻辑可以复用。代码 4-24 给出了这一过程的删节实现。

```cpp
Handle<PyModule> ModuleImporter::ImportModule(...) {
  ...
  Handle<PyString> fullname =
      manager_->name_resolver()->ResolveFullName(name, level, globals);
  ...
  Handle<PyModule> last_module = ImportModuleImpl(fullname);
  ...
}

Handle<PyModule> ModuleImporter::ImportModuleImpl(Handle<PyString> fullname) {
  Handle<PyModule> last_module;
  ...
  // 遍历 fullname，分段进行导入
  while (...) {
    ...
    Handle<PyModule> part_module = GetOrLoadModulePart(...);
    ...
    last_module = part_module;
    ...
  }
  return last_module;
}
```
代码 4-24

最后，分段后导入具体的包或者模块是在 `ModuleImporter::GetOrLoadModulePart()` 当中完成的：首先检查模块缓存；若目标模块已经加载，则直接复用现有对象；否则再调用 `ModuleLoader` 组件进入后续的路径查找和模块加载流程。代码 4-25 给出了这一过程的删节实现。

```cpp
Handle<PyModule> ModuleImporter::GetOrLoadModulePart(
    Handle<PyString> part_fullname, ...) {
  // Fast Path: 模块已经被缓存
  Handle<PyObject> cached = PyDict::Get(modules_dict(), part_fullname, ...);
  if (!cached.is_null()) {
    return Handle<PyModule>::cast(cached);
  }
  // Slow Path: 走 module loader 加载模块
  ...
  return manager_->loader()->LoadModulePart(part_fullname, search_path_list);
}
```
代码 4-25

### 4.4.4 代码装载前端实现

对本文课题而言，重点研究的对象是 PVM 后端，因此 S.A.A.U.S.O VM 在源码编译能力上直接封装了 CPython 的编译前端；关闭该复用后，PVM 后端仍可独立运行，但部分依赖源码编译前端的功能会受到限制。与此同时，系统也支持直接装载 CPython 编译器前端生成的 `.pyc` 格式代码对象二进制文件，从而为脚本执行提供源码与字节码两类输入入口。

## 4.5 字节码解释器与异常处理的实现

在建立起执行门面、运行时语义、内建能力和代码装载前端等基础设施后，就可以开始搭建字节码解释器了。受限于论文篇幅，本节不逐条罗列全部字节码指令，而只选取第 2 章讨论过的几类核心机制进行说明，包括函数对象表示、栈帧组织、命名空间查找、函数调用主路径、调度表解释器以及异常展开机制。

### 4.5.1 Python 函数对象与可调用对象表示

在 2.3.1 中已经指出，Python 中的函数的本质是运行阶段由 PVM 动态创建的可调用对象。在现阶段 S.A.A.U.S.O VM 的实现中，普通 Python 函数与 native 函数统一由 `PyFunction` 对象表示，只是在其内部保存的元数据和实际调用路径上有所区别。

代码 4-27 给出了 `PyFunction` 的删节定义。

```cpp
class PyFunction : public PyObject {
 private:
  Tagged<PyObject> func_code_;
  Tagged<PyObject> func_globals_;
  Tagged<PyObject> default_args_;
  Tagged<PyObject> closures_;
  NativeFuncPointer native_func_{nullptr};
};
```
代码 4-27

这段代码说明，`PyFunction` 至少承担四类信息的组织责任：
（1）`func_code_` 对应函数的静态代码对象；
（2）`func_globals_` 对应函数创建时绑定的全局命名空间；
（3）`default_args_` 与 `closures_` 分别对应全体默认参数和全体被该函数捕获的自由变量；
（4）`native_func_` 对应 native 函数的 C++ 函数指针。

其中，（1）（2）（3）服务于普通 Python 函数，（4）预留给 native 函数使用。与此同时，`MAKE_FUNCTION` 的实现也与 2.3.1 中的理论分析一致：解释器根据代码对象动态创建 `PyFunction`，再把当前栈帧中的全局变量表、默认参数和自由变量元组写入其中。

代码 4-28 给出了 `MAKE_FUNCTION` 字节码处理逻辑的删节实现。

```cpp
INTERPRETER_HANDLER_WITH_SCOPE(MakeFunction, {
  auto code_object = Handle<PyCodeObject>::cast(POP());
  Handle<PyFunction> func =
      isolate_->factory()->NewPyFunctionWithCodeObject(code_object);
  func->set_func_globals(current_frame_->globals(isolate_));

  if (op_arg & MakeFunctionOpArgMask::kClosure) {
    func->set_closures(Handle<PyTuple>::cast(POP()));
  }
  if (op_arg & MakeFunctionOpArgMask::kDefaults) {
    func->set_default_args(Handle<PyTuple>::cast(POP()));
  }
  PUSH(func);
})
```
代码 4-28

### 4.5.2 栈帧数据结构与局部执行现场

在 S.A.A.U.S.O VM 中，栈帧的实体为 `FrameObject` 类。代码 4-29 给出了该类的删节定义。

```cpp
class FrameObject : Object {
 private:
  Tagged<PyObject> stack_;

  Tagged<PyObject> locals_;
  Tagged<PyObject> localsplus_;
  Tagged<PyObject> globals_;

  Tagged<PyObject> code_object_;
  Tagged<PyObject> func_;
  int64_t pc_{0};
  FrameObject* caller_;
};
```
代码 4-29

从这一定义可以看出，除了 2.3.2 中讨论过的操作数栈、程序计数器以及 `locals`、`globals` 与 `localsplus` 三类环境，S.A.A.U.S.O VM 的栈帧还显式保存了当前函数对象、代码对象和上一层调用者指针 `caller_`。这意味着本文系统中的 Python 调用栈并不是隐含在 C++ 调用栈中，而是由 PVM 自己维护。

下面再进一步说明这些变量环境是如何被真正装配进栈帧的。在现阶段的 S.A.A.U.S.O VM 中，当解释器需要发起 Python 函数调用时，栈帧的初始化及装配由 `FrameObjectBuilder` 承担。代码 4-30 给出了这部分逻辑的删节实现。

```cpp
struct FrameBuildContext {
  Handle<PyDict> locals;
  Handle<PyDict> globals;
  Handle<FixedArray> localsplus;
  Handle<PyCodeObject> code_object;
  Handle<PyFunction> func;
};

FrameBuildContext PrepareForFunction(Isolate* isolate,
                                    Handle<PyFunction> func,
                                    Handle<PyDict> bound_locals) {
  FrameBuildContext ctx;
  ctx.code_object = func->func_code(isolate);
  ctx.func = func;
  ctx.locals = bound_locals;
  ctx.globals = func->func_globals(isolate);
  isolate->factory()->NewFixedArray(ctx.code_object->nlocalsplus());
  ctx.stack = isolate->factory()->NewFixedArray(ctx.code_object->stack_size());
  ...
  return ctx;
}

FrameObject* FrameObjectBuilder::BuildSlowPath(
    Isolate* isolate,
    Handle<PyFunction> func,
    Handle<PyDict> bound_locals,
    ...) {
  FrameBuildContext ctx =
      PrepareForFunction(isolate, func, bound_locals);
  ...
  auto* frame_object = FrameObject::Create(ctx);
  return frame_object;
}
```
代码 4-30

从这段实现可以看到，本文系统中的栈帧装配策略与 2.3.2 的理论分析是一致的：每个函数栈帧都显式保存其绑定的全局命名空间；调用时可以灵活指定 `locals`；`localsplus` 则按代码对象中记录的槽位数单独分配，用于承载形参、普通局部变量和自由变量。

### 4.5.3 局部变量、全局变量与名称查找

在建立了栈帧的数据结构后，下一步就可以给出变量读写相关字节码指令的具体实现了。

代码 4-31 给出了 `STORE_NAME/LOAD_NAME` 字节码指令的删节实现。

```cpp
INTERPRETER_HANDLER_WITH_SCOPE(StoreName, {
  Handle<PyObject> key =
      current_frame_->names(isolate_)->Get(op_arg, isolate_);
  Handle<PyDict> locals = current_frame_->locals(isolate_);
  ...
  PyDict::Put(locals, key, POP(), isolate_);
})

INTERPRETER_HANDLER_WITH_SCOPE(LoadName, {
  Handle<PyObject> key =
      current_frame_->names(isolate_)->Get(op_arg, isolate_);
  ...
  Handle<PyObject> value = 
      PyDict::Get(current_frame_->locals(isolate_), key, ...);
  if (!value.is_null()) { PUSH(value); break; }

  value = PyDict::Get(current_frame_->globals(isolate_), key, ...);
  if (!value.is_null()) { PUSH(value); break; }

  value = PyDict::Get(isolate_->builtins(), key, ...);
  if (!value.is_null()) { PUSH(value); break; }
})
```
代码 4-31

对于 `STORE_NAME`，系统先取出符号名，再把写入操作落到 `locals` 字典上；对于 `LOAD_NAME`，若 `locals` 未命中，则继续向 `globals` 和内建命名空间回退。这与第 2 章给出的名称查找规则一致。

代码 4-32 给出了 `STORE_GLOBAL/LOAD_GLOBAL` 和 `STORE_FAST/LOAD_FAST` 的删节实现。它们与 2.3.2 中的理论分析基本一致，因此不再展开。

```cpp
INTERPRETER_HANDLER_WITH_SCOPE(StoreGlobal, {
  Handle<PyObject> key =
      current_frame_->names(isolate_)->Get(op_arg, isolate_);
  PyDict::Put(current_frame_->globals(isolate_), key, POP(), isolate_);
})

INTERPRETER_HANDLER_WITH_SCOPE(LoadGlobal, {
  Handle<PyObject> key =
      current_frame_->names(isolate_)->Get(op_arg >> 1, isolate_);
  ...
  Handle<PyObject> value = 
      PyDict::Get(current_frame_->globals(isolate_), key, ...);
  if (!value.is_null()) { PUSH(value); break; }

  value = PyDict::Get(isolate_->builtins(), key, ...);
  if (!value.is_null()) { PUSH(value); break; }
})

INTERPRETER_HANDLER_WITH_SCOPE(
    StoreFast, current_frame_->localsplus(isolate_)->Set(op_arg, *POP());)
INTERPRETER_HANDLER_WITH_SCOPE(
    LoadFast, PUSH(current_frame_->localsplus(isolate_)->Get(op_arg));)
```
代码 4-32

### 4.5.4 函数闭包的实现

在 2.3.3 中已经分析了函数闭包机制的实现原理。本节中将围绕 `MAKE_FUNCTION`、`MAKE_CELL`、`LOAD_CLOSURE`、`COPY_FREE_VARS`、`LOAD_DEREF` 和 `STORE_DEREF` 这几条关键字节码指令，给出 S.A.A.U.S.O VM 中函数闭包机制的实际实现。相关删节代码见代码 4-33。

```cpp
INTERPRETER_HANDLER_WITH_SCOPE(MakeFunction, {
  ...
  if (op_arg & MakeFunctionOpArgMask::kClosure) {
    func->set_closures(Handle<PyTuple>::cast(POP()));
  }
  ...
})

INTERPRETER_HANDLER_WITH_SCOPE(MakeCell, {
  Handle<Cell> cell = isolate_->factory()->NewCell();
  Tagged<PyObject> initial =
      current_frame_->localsplus(isolate_)->Get(op_arg);
  cell->set_value(initial);
  current_frame_->localsplus(isolate_)->Set(op_arg, cell);
})

INTERPRETER_HANDLER_DISPATCH(LoadClosure, {
  Tagged<PyObject> cell = current_frame_->localsplus(isolate_)->Get(op_arg);
  PUSH(cell);
})

INTERPRETER_HANDLER_DISPATCH(CopyFreeVars, {
  Tagged<PyTuple> func_closures = current_frame_->func_tagged()->closures_tagged();
  ...
  for (...) {
    current_frame_->localsplus(isolate_)->Set(..., func_closures->GetTagged(i));
  }
})

INTERPRETER_HANDLER_WITH_SCOPE(LoadDeref, {
  Tagged<Cell> cell = current_frame_->localsplus(isolate_)->Get(op_arg);
  Tagged<PyObject> value = cell->value_tagged();
  PUSH(value);
  ...
})

INTERPRETER_HANDLER_DISPATCH(StoreDeref, {
  Tagged<PyObject> value = POP_TAGGED();
  Tagged<Cell> cell = current_frame_->localsplus(isolate_)->Get(op_arg);
  cell->set_value(value);
})
```
代码 4-33

从这段代码看，闭包机制的具体实现与 2.3.3 中给出的理论分析是一致的。首先，`MAKE_CELL` 把自由变量从普通槽位改为由单元对象间接持有；其次，`LOAD_CLOSURE` 与 `MAKE_FUNCTION` 把这些单元对象绑定进内部函数；最后，`COPY_FREE_VARS`、`LOAD_DEREF` 与 `STORE_DEREF` 保证新栈帧继续访问并更新同一组单元对象。

### 4.5.5 函数调用主路径与调用栈管理

函数调用是第 2 章最强调的核心机制之一。对于 S.A.A.U.S.O VM 而言，这一过程既包括普通 Python 函数的建帧与解释执行，也包括 native 函数的直接调用路径，还包括 Python 调用栈本身如何被创建、维护和回退。

代码 4-34 给出了相关实现的删节代码。

```cpp
Handle<PyObject> Interpreter::CallPythonImpl(Handle<PyObject> callable,
                                             Handle<PyObject> receiver,
                                             Handle<PyTuple> pos_args,
                                             Handle<PyDict> kw_args) {
  EscapableHandleScope scope(isolate_);
  ...
  Handle<PyObject> result;
  if (IsNormalPyFunction(callable, isolate_)) {
    FrameObject* frame =
        FrameObjectBuilder::BuildSlowPath(
            isolate_, callable, receiver, pos_args, kw_args);
    ...
    EnterFrame(frame);
    EvalCurrentFrame();

    Handle<PyObject> result = ReleaseReturnValue();
    DestroyCurrentFrame();
    ...
    return scope.Escape(result);
  }

  result = CallNonNormalFunction(callable, receiver, pos_args, kw_args);
  return scope.Escape(result);
}

void Interpreter::EnterFrame(FrameObject* frame) {
  frame->set_caller(current_frame_);
  current_frame_ = frame;
}

void Interpreter::DestroyCurrentFrame() {
  FrameObject* callee = current_frame_;
  current_frame_ = callee->caller();
  delete callee;
}
```
代码 4-34

在这段代码中，`Interpreter::CallPythonImpl()` 是 S.A.A.U.S.O VM 中执行调用操作的底层逻辑中枢。其中：
（1）`callable` 参数表示一个可调用 Python 对象，它可能是一个普通的 Python 函数对象，也可能是其他类别的可调用对象，比如一个包装成 `PyFunction` 的 native 函数，或者重载了 `__call__` 魔法函数的用户自定义类实例。
（2）`receiver` 参数仅在 PVM 执行对象方法调用时有效，表示发起方法调用的 Python 对象。
（3）`pos_args` 和 `kw_args` 分别表示用户程序传入的位置参数和键值对参数。

这段代码说明了普通 Python 函数调用的关键步骤：构造新栈帧并完成参数绑定；通过 `EnterFrame()` 将其加入 PVM 调用链；调用 `EvalCurrentFrame()` 逐条解释执行；最后提取返回值并通过 `DestroyCurrentFrame()` 恢复上一层栈帧。

与此同时，`CallPythonImpl()` 也体现出普通 Python 函数与其他可调用对象的执行分流：若目标属于 native 函数或其他可调用对象，则转入 `CallNonNormalFunction()`。这样，上层逻辑只需依赖统一调用入口，而无需关心可调用对象的具体身份。

代码 4-35 给出了 `CallNonNormalFunction()` 内部逻辑的删节实现。

```cpp
Handle<PyObject> Interpreter::CallNonNormalFunction(...) {
  // 如果是NativeFunction，直接执行调用
  if (IsNativePyFunction(callable, isolate_)) {
    return Runtime_CallNativePyFunction(isolate_, callable, ...);
  }
  // 否则尝试调用callable的call虚方法
  return PyObject::Call(isolate_, callable, ...);
}

Handle<PyObject> Runtime_CallNativePyFunction(...) {
  ...
  NativeFuncPointer native_func_ptr = func->native_func();
  ...
  return native_func_ptr(isolate, receiver, args, kwargs);
}

Handle<PyObject> PyObject::Call(...) {
  Tagged<Klass> klass = ResolveObjectKlass(self, isolate);
  auto* call_method = klass->vtable().call_;
  return call_method(isolate, self, ...);
}
```
代码 4-35

### 4.5.6 基于调度表的字节码分派

在建立起函数对象、栈帧和变量访问机制之后，就可以搭建解释器“逐条调度执行字节码指令”的主路径了。代码 4-36 给出了实际的 S.A.A.U.S.O VM 中，调度和控制流相关逻辑的删节实现。

```cpp
void Interpreter::EvalCurrentFrame() {
  uint8_t op_code = 0;
  int op_arg = 0;

#define INTERPRETER_HANDLER_DISPATCH(bytecode, ...) \
  handler_##bytecode {                              \
    do {                                            \
      __VA_ARGS__                                   \
    } while (0);                                    \
    DISPATCH();                                     \
  }

#define DISPATCH()                                      \
  do {                                                  \
    if (isolate_->HasPendingException()) [[unlikely]] { \
      goto pending_exception_unwind;                    \
    }                                                   \
    if (!current_frame_->HasMoreCodes()) [[unlikely]] { \
      goto exit_interpreter;                            \
    }                                                   \
    op_code = current_frame_->GetOpCode();              \
    op_arg = current_frame_->GetOpArg();                \
    goto* dispatch_table_[op_code];                     \
  } while (0)

INTERPRETER_HANDLER_DISPATCH(PopJumpIfFalse, {
  Tagged<PyObject> condition = POP_TAGGED();
  if (!Runtime_PyObjectIsTrue(isolate_, condition)) {
    current_frame_->set_pc(current_frame_->pc() + (op_arg << 1));
  }
})

其他字节码指令的实现略...
}

uint8_t FrameObject::GetOpCode() {
  ...
  return bytecodes->buffer()[pc_++];
}
```
代码 4-36

从这段代码可以看出，本文系统并非通过巨大的 `switch` 语句分发字节码，而是通过分发表（dispatch table）直接跳转到对应 handler。每条指令执行完后，`DISPATCH()` 统一处理三类状态：待处理异常、当前栈帧结束以及下一条字节码的继续调度。为减少重复代码，系统又引入 `INTERPRETER_HANDLER_DISPATCH` 宏来统一封装单条指令的 handler 结构。

其中 `POP_JUMP_IF_FALSE` 的实现也直接回应了 2.2 的理论分析：跳转指令通过修改程序计数器改变控制流，而条件判断则依赖运行时提供的对象真值语义。

### 4.5.7 异常机制的实现

在 2.5 中已经指出，异常机制本质上是解释器主路径中的一种特殊控制流，因此它不能被理解为解释器之外附加的一段错误处理逻辑。同时，要实现一个实际可用的异常机制，PVM 至少需要解决异常状态记录、异常表查找与栈展开两大关键问题。

这里首先讨论异常状态记录的实现。在本文系统中，异常状态是由 `ExceptionState` 组件统一管理的，代码 4-37 给出了它的删节定义。其中 `pending_exception_` 字段用于保存异常对象，`pending_exception_pc_` 字段用于保存触发异常的字节码指令的地址。

```cpp
class Isolate {
 public:
  ExceptionState* exception_state() { return &exception_state_; }
 private:
  ExceptionState exception_state_;
};

class ExceptionState final {
 public:
  bool HasPendingException() const { return !pending_exception_.is_null(); }
  ...
  void Throw(Handle<PyBaseException> exception);
  void Clear();

 private:
  Tagged<PyObject> pending_exception_;
  int pending_exception_pc_;
  ...
};
```
代码 4-37

从这段代码中可以看到，`ExceptionState` 被挂在 `Isolate` 实例上，而没有被收缩为解释器内部的私有组件。这样设计的好处有两点：其一，便于把异常状态作为 VM 实例级别的共享状态统一维护；其二，为运行时语义、内建能力和解释器等子系统提供统一接口，用于向 VM 抛出异常或查询异常状态，从而避免不同模块各自维护一套异常表示。

接下来再讨论异常表查找与栈展开的实现。在 4.5.6 中已经提到，当解释器在 `DISPATCH()` 中检测到 VM 存在待处理的异常状态时，会立即跳入 `pending_exception_unwind` 以处理异常。代码 4-38 给出了这部分逻辑的删节实现。

```cpp
void Interpreter::EvalCurrentFrame() {
  ...
pending_exception_unwind: {
  auto* exception_state = isolate_->exception_state();
  exception_state->set_pending_exception_pc(current_frame_->pc() -
                                            kBytecodeSizeInBytes);

  ExceptionHandlerInfo handler_info;
  if (ExceptionTable::LookupHandler(
          isolate_, current_frame_->code_object(isolate_),
          exception_state->pending_exception_pc(), handler_info)) {
    current_frame_->set_stack_top(handler_info.stack_depth);
    ...
    PUSH(exception_state->pending_exception_tagged());
    exception_state->Clear();
    current_frame_->set_pc(handler_info.handler_pc);
    DISPATCH();
  }

  ...
  UnwindCurrentFrameForException();
  DISPATCH();
}
  ...
}
```
代码 4-38

从这段代码中可以看到，`pending_exception_unwind` 主要承担异常表查找与当前帧内控制流恢复的职责。首先，解释器会记录触发异常的字节码指令地址；其次，调用 `ExceptionTable::LookupHandler()`，依据该地址查询当前栈帧对应代码对象的异常表中是否存在覆盖该位置的处理器（handler）。若存在有效处理器，解释器便先恢复处理器要求的操作数栈深度，再把异常对象压入栈顶，随后把程序计数器指向目标处理器的入口地址，并借助 `DISPATCH()` 宏完成跳转。在目标处理器中，异常对象作为栈顶值一般会被消费。相对地，若当前栈帧中不存在匹配处理器，则解释器转而调用 `UnwindCurrentFrameForException()` 执行栈帧回溯。

在代码 4-39 中给出了 `UnwindCurrentFrameForException()` 的删节实现。从中可以看到，它会销毁当前栈帧，并将异常继续向调用方函数的栈帧传播。这一过程正对应第 2 章伪代码中的“弹出当前栈帧并继续向外层查找处理器”。进一步地，在代码 4-38 中调用 `UnwindCurrentFrameForException()` 完毕后，由于 VM 的异常状态仍未被清除，且 `ExceptionState` 中记录的异常发生地址已经被回溯为上一层栈帧中的函数调用点地址，因此借助 `DISPATCH()` 宏，解释器就可以在调用方栈帧中再次尝试查询异常表；如此循环往复，直至找到有效的异常处理器，或回溯到根栈帧并迫使 VM 停止继续执行整个 Python 程序。

```cpp
void Interpreter::UnwindCurrentFrameForException() {
  // 当前栈帧已经发生错误，因此强制回溯到上一层栈帧
  DestroyCurrentFrame();
  ret_value_ = Tagged<PyObject>::null();
  // 重要：将 pending_exception_pc 回溯为上一层栈帧的函数调用点地址
  if (current_frame_ != nullptr) {
    isolate_->exception_state()->set_pending_exception_pc(current_frame_->pc() -
                                                          kBytecodeSizeInBytes);
  }
  ...
}
```
代码 4-39

概括而言，`pending_exception_unwind` 与 `UnwindCurrentFrameForException()` 相结合，在本文系统中落实了 2.5 中给出的栈展开原理。通过它们与中心化异常状态组件 `ExceptionState` 的配合，在工程上实现了异常机制的主路径。

## 4.6 Embedder API 实现

除了作为独立解释执行系统之外，S.A.A.U.S.O VM 还希望具备较好的嵌入能力。因此，系统设计并实现了一套面向宿主程序的 Embedder API，用于把 VM 核心能力以更稳定、易用的方式暴露给外部 C++ 应用。总体上，该接口采用“公共头文件 + API 桥接层 + VM 内核实现”的三层结构：最外层面向宿主公开稳定抽象，中间层负责把公开接口转换为 VM 内部可理解的表示，最内层则复用前文介绍的解释器、运行时和对象系统等内核能力。借助这种边界划分，VM 的内部对象布局、GC 细节与解释器状态不会直接泄漏给外部调用者，从而使嵌入接口能够在保持可用性的同时维持相对稳定的边界。

在这套接口中，`Isolate` 作为独立 VM 实例与运行时容器对外暴露，`Context` 表示脚本运行所依赖的上下文环境，而 `Script`、`Value`、`Function` 与 `TryCatch` 等抽象则分别承担脚本装载、值交换、函数调用和异常观测等职责。宿主程序通常先创建 `Isolate`，再在其内部建立 `Context`，随后完成脚本编译与执行；这意味着宿主方只需围绕 VM 实例、上下文环境和脚本对象三个核心概念组织流程，而无须直接依赖 VM 内部实现细节。

进一步地，S.A.A.U.S.O VM 的 Embedder API 并不只支持“宿主驱动脚本执行”，还支持宿主与脚本之间的双向互操作：一方面，宿主程序可以创建函数对象并注入脚本环境，使 Python 脚本调用 C++ 回调；另一方面，宿主程序也可以从脚本上下文中取回 Python 函数对象，并在 C++ 侧发起调用。这说明前文构建的对象系统、函数调用机制与异常传播机制，不仅能够支撑 VM 内部执行，也能够被进一步封装为宿主程序可直接利用的脚本引擎能力。

## 4.7 本章小结

本章按照自底向上的顺序，对第 3 章提出的总体设计进行了落地。这些核心模块虽然职责分离，但共同构成了一个协同工作的整体：堆提供托管内存与 GC 基础，句柄保证 native C++ 持有引用的安全性，对象系统统一表示 Python 对象及其类型行为，执行层基础设施提供程序运行环境，而解释器与异常机制则真正推动程序执行与控制流恢复。

更重要的是，第 2 章提出的核心理论问题已经在本章获得了明确实现：命名空间与变量绑定落地为栈帧及相关字节码指令；函数对象、函数调用与闭包落地为 `PyFunction`、`FrameObject` 和相关链路；异常机制落地为 `ExceptionState`、异常表查询和栈展开主路径。换言之，第 2 章回答了“要解决什么问题”，本章回答了“这些问题如何在系统中实现”。

基于现阶段 S.A.A.U.S.O 的实现基础，下一章将进一步通过测试与结果分析，对系统的功能正确性、兼容性和当前表现进行验证。

# 第5章 S.A.A.U.S.O VM 功能测试与结果分析

本章围绕功能正确性、字节码模型兼容性、运行时稳定性以及 Embedder API 可用性，对现阶段的 VM 系统进行测试与结果分析。由于当前课题目标是建立一个兼容 CPython 3.12 核心字节码模型的最小 VM 闭环，因此测试重点放在功能正确性与运行稳定性上，而不进行与成熟工业级 PVM 的大规模性能对比。

## 5.1 测试目标

结合第 1 章中提出的课题目标，以及第 2 章至第 4 章中给出的理论分析与工程实现，本文将现阶段 S.A.A.U.S.O VM 的测试目标归纳为以下四个方面。

第一，验证字节码解释执行主路径的功能正确性。具体包括控制流跳转、变量读写、函数调用、闭包、异常传播等核心语义，确保它们在当前系统中能够被正确解释执行。

第二，验证对象系统与内建运行环境的可用性。具体包括对象属性访问、方法绑定、内建类型构造、容器读写、运算协议、模块导入以及若干关键内建模块的行为，确认系统已经具备支撑常用 Python 程序的基础能力。

第三，验证运行时基础设施的稳定性。具体包括句柄机制与 GC 机制的协同、在 GC 压力下对象引用是否能被正确维护、异常系统是否能跨栈帧正确展开，以及在复杂语义组合场景中系统是否仍能保持一致行为。

第四，验证 Embedder API 的工程可用性。由于 S.A.A.U.S.O VM 的关键目标之一是作为轻量级脚本引擎嵌入宿主 C++ 程序，因此还需要确认 4.6 中介绍的 `Isolate/Context/Script/TryCatch/Function` 等 API 是否能按预期工作。

## 5.2 测试内容与方法

### 5.2.1 测试内容

考虑到 CPython 官方测试集中包含大量超出课题研究范围的高阶语言功能，现阶段 S.A.A.U.S.O VM 基于 Google Test 自动化测试框架自行维护了两套更贴合课题目标的单元测试集。

第一套为针对 VM 内核的单元测试集，位于项目 `test/unittests/` 目录下。这部分测试直接覆盖虚拟机堆、对象系统、句柄机制、字节码解释器、异常系统、模块系统、编译前端和若干工具组件，是验证本文系统正确性的主要依据。

第二套为针对 Embedder API 的单元测试集合，位于项目 `test/embedder/` 目录下。这部分测试从宿主程序视角验证公开嵌入接口，包括 `Isolate` 生命周期、上下文管理、脚本执行、宿主与脚本的双向互操作等内容。

表 5-1 给出了这两套测试集的量化统计情况。

|测试集|测试源文件个数|测试用例数|
|-----|-------------|---------|
|针对 VM 核心| 62 个 | 392 个 |
|针对 Embedder API | 7 个 | 46 个 |

表 5-1

这说明现阶段 S.A.A.U.S.O VM 已具备较为系统化的自动化回归基础。5.3 和 5.4 将进一步说明其覆盖内容。

### 5.2.2 测试方法

现阶段 S.A.A.U.S.O VM 已将 Google Test 与全部单元测试集成到项目中。执行测试时，先运行项目根目录下的 `build.sh` 构建 VM 内核与测试程序，再分别运行 `ut.exe` 和 `embedder_ut.exe` 即可批量获取结果。

## 5.3 功能测试覆盖的内容

### 5.3.1 字节码解释执行与控制流

在字节码解释执行方面，现阶段测试已覆盖变量读写、条件分支、循环跳转等基础路径。

相关源文件包括：
- `interpreter-smoke-unittest.cc`
- `interpreter-controlflow-unittest.cc`

### 5.3.2 函数调用、参数绑定与闭包

函数调用与闭包部分较易出现语义偏差，因此当前测试覆盖了位置参数、关键字参数、默认参数、`*args/**kwargs` 展开、参数错误、方法调用时 `self` 注入、递归调用以及与闭包等场景。

相关源文件主要包括：
- `interpreter-functions-args-binding-unittest.cc`
- `interpreter-functions-call-errors-unittest.cc`
- `interpreter-functions-call-unpack-unittest.cc`
- `interpreter-functions-recursion-unittest.cc`
- `interpreter-functions-varargs-def-unittest.cc`
- `interpreter-closure-unittest.cc`

### 5.3.3 对象系统、内建类型与运行时行为

对象系统方面，现阶段测试已覆盖对象属性、实例属性与类属性查找、方法绑定、自定义类构造、MRO、多态行为以及内建类型的构造与核心能力分派等内容。

相关源文件主要包括：
- `interpreter-attribute-constraints-unittest.cc`
- `interpreter-builtins-constructor-unittest.cc`
- `interpreter-builtins-method-dispatch-unittest.cc`
- `interpreter-custom-class-core-unittest.cc`
- `interpreter-custom-class-construct-unittest.cc`
- `interpreter-custom-class-dunder-unittest.cc`
- `interpreter-custom-class-mro-unittest.cc`

### 5.3.4 异常机制与模块系统

异常机制方面，现阶段测试已覆盖异常对象格式化、`try/except/finally`、`raise/from`、异常匹配、跨函数栈展开以及 `StopIteration` 与迭代器语义协同等内容。

相关源文件主要包括：
- `interpreter-exceptions-format-unittest.cc`
- `interpreter-exceptions-iteration-unittest.cc`
- `interpreter-exceptions-trycatch-unittest.cc`
- `interpreter-exceptions-unwind-unittest.cc`

模块系统方面，现阶段测试已经覆盖模块缓存、包导入、相对导入和全量导入等内容。相关源文件主要包括 `interpreter-import-unittest.cc`。

## 5.4 运行时稳定性与 Embedder API 测试覆盖的内容

### 5.4.1 句柄、GC 与运行时稳定性

在运行时基础设施方面，本文重点验证句柄系统与 GC 的协同是否可靠。为此，当前项目构建了针对局部句柄、长期句柄、跨线程边界、GC 根遍历、移动后引用更新以及 GC 压力场景的测试。

例如，在 `gc-unittest.cc`、`gc-interpreter-stress-unittest.cc`、`global-handle-unittest.cc`、`handle-unittest.cc` 等源文件中，重点验证了对象在经历 Scavenge GC 后引用仍然可达、句柄槽位能够随对象移动而被更新，以及 VM 在 GC 压力条件下不会错误丢失活对象；此外，还引入了解释器层测试以确认内建对象在显式 GC 压力下仍能保持功能正常。

### 5.4.2 Embedder API 可用性验证

除了 VM 内核本身，Embedder API 也是本文的重要验收对象。当前 `test/embedder/` 共包含 7 个测试源文件、46 个 GTest 用例，覆盖 `Isolate` 创建与销毁、`Context` 创建与使用、脚本编译与执行、宿主与脚本的双向互操作、TryCatch 异常捕获等内容，同时也验证了接口在错误使用时能否及时拦截，例如在未正确创建 `Isolate` 或跨线程非法访问时，debug 版本会立即触发断言。

## 5.5 测试结果总结与分析

### 5.5.1 测试结果

在本次论文撰写阶段的完整测试中，先成功构建了 VM 内核与单元测试执行程序，再分别运行核心 VM 测试与 Embedder API 测试。结果如表 5-2 所示。

| 测试目标 | 测试用例数 | 结果 | 总耗时 |
| --- |  --- | --- | --- |
| 核心 VM 测试 `ut.exe` |  392 | 全部通过 | 4554 ms |
| Embedder API 测试 `embedder_ut.exe` |  46 | 全部通过 | 1082 ms |
| 合计 |  438 | 全部通过 | 5636 ms |

表 5-2

从表 5-2 可以看到，现阶段 S.A.A.U.S.O VM 在一次完整回归中通过了全部 438 个自动化测试用例，没有出现构建、链接或测试失败的情况。这说明系统当前已经具备较好的整体一致性。

### 5.5.2 结果分析

结合 5.3 与 5.4 的覆盖情况，可以从三个层面理解这些结果。

第一，从功能正确性看，当前系统已经围绕控制流、函数调用、闭包、对象系统、内建类型、异常机制和导入系统建立了较完整的回归网，因此第 4 章给出的实现已经能够支撑 Python 核心功能子集。

第二，从兼容性看，当前系统虽然尚未覆盖现代 Python 的全部特性，但围绕“兼容 CPython 3.12 核心字节码模型”这一目标，已经完成了闭包、异常表展开、模块导入、方法绑定和自定义类等主路径验证，说明本系统已初步具备处理真实 Python 程序关键语义的能力。

第三，从稳定性看，压力测试结果表明，现阶段解释器、GC 与对象句柄机制等核心设施是稳定且可控的，这也是一个可实际使用的 PVM 所必须具备的基础条件。

## 5.6 本章小结

本章围绕功能正确性、兼容性、运行时稳定性与 Embedder API 可用性，对现阶段 S.A.A.U.S.O VM 进行了系统测试与结果分析。测试结果表明，当前系统通过了全部 438 个自动化测试用例，已经具备轻量级 PVM 应有的最小可用闭环，基本达成了 1.3.2 中提出的研究目标。

# 第6章 总结与展望

## 6.1 工作总结

本文围绕“一个轻量级 Python 虚拟机的设计与实现”这一课题，设计并实现了一个名为 S.A.A.U.S.O VM 的轻量级 PVM 后端系统。该系统以教学与嵌入场景为主要目标，采用 C++20 实现，在不完整复刻 CPython 的前提下，围绕字节码解释执行、对象系统、运行时机制和嵌入接口等关键问题，建立了一个兼容 CPython 3.12 核心字节码模型的最小功能闭环。

在理论分析方面，本文首先从 PVM 执行模型出发，围绕函数对象、栈帧、闭包、对象系统、异常机制和模块机制等核心问题，梳理了一个轻量级 PVM 后端在实现 Python 语言核心功能子集时需要解决的关键语义问题，并据此构建了后续系统设计与实现的分析主线。第 2 章的相关讨论，一方面明确了本文系统需要具备的核心能力，另一方面也为第 3 章和第 4 章中的工程落地提供了理论基础。

在总体设计方面，本文围绕轻量化、可嵌入和工程可维护三个目标，对 S.A.A.U.S.O VM 的系统结构进行了统一设计。系统以 `Isolate` 作为核心运行时容器，在其上组织虚拟机堆、句柄机制、对象表示层、执行层与嵌入接口层等关键模块，并通过清晰的职责边界和依赖关系，将对象管理、解释执行、异常传播、模块导入和宿主互操作等能力统一收敛到同一套运行时框架之下。这样的总体设计使系统在保持功能闭环的同时，仍然能够维持较好的结构清晰度与后续可演进性。

在关键模块实现方面，本文完成了轻量级 PVM 所需的一组核心子系统。首先，在内存管理方面，系统实现了虚拟机堆、新生代空间与基于 Cheney 算法的 Scavenge GC 主路径，并通过页化空间为后续老生代与更完整的分代式回收预留了结构基础。其次，在对象系统方面，本文实现了基于 `PyObject` 与 `Klass` 分工的统一对象表示机制，将对象实体数据、类型元信息、属性查找和核心行为分派组织到一套相互配合的对象模型之中，同时完成了若干基础内建类型与用户自定义类的支持。再次，在执行层方面，系统实现了以分发表驱动的字节码解释器、显式维护的 Python 栈帧、函数调用与参数绑定主路径、闭包支持、异常状态管理与栈展开机制，并实现了模块导入与代码装载相关基础设施。最后，在系统对外能力方面，本文还设计并实现了面向宿主 C++ 程序的 Embedder API，使 S.A.A.U.S.O VM 不仅能够独立执行脚本，也能够作为轻量级脚本引擎嵌入其他应用。

在测试与验证方面，本文基于 Google Test 构建了针对 VM 内核与 Embedder API 的两套自动化测试集，并围绕解释执行、对象系统、内建能力、异常机制、模块系统、句柄与 GC 协同以及宿主互操作等方面进行了系统验证。测试结果表明，现阶段 S.A.A.U.S.O VM 已通过 438 个自动化测试用例，说明系统已经具备较好的功能正确性、运行稳定性与工程一致性，基本达成了第 1 章中提出的研究目标。

## 6.2 创新点总结

综合全文内容，本文工作的创新性并不在于简单复现一个现成的 Python 实现，而在于围绕“轻量级、可嵌入、可教学演示”的目标，对 Python 虚拟机后端的若干关键问题进行了重新组织，并形成了一个真实可运行、可测试的系统实现。具体而言，本文的主要创新点可以概括为以下几个方面。

第一，本文面向教学与嵌入场景，设计并实现了一个结构清晰、规模可控的轻量级 PVM 后端系统。与以完整生态和工业成熟度为主要目标的 CPython 不同，本文并不追求覆盖全部 Python 语义，而是围绕核心功能子集建立一个可运行、可验证、便于阅读与修改的最小系统闭环。这种面向特定目标进行系统裁剪和重构的实现方式，使本文系统在教学展示和二次开发方面具有较强针对性。

第二，本文围绕 CPython 3.12 核心字节码模型，建立了一套从理论分析到工程实现相互对应的解释执行框架。系统不仅实现了字节码分发表解释器、显式 Python 栈帧和函数调用主路径，还进一步支持了闭包、异常表驱动的异常展开以及模块导入等更具代表性的 Python 核心语义，从而使系统不再停留于仅能运行简单表达式或控制流的演示级解释器，而是具备处理真实 Python 程序关键语义的初步能力。

第三，本文针对 Python 动态语言特征，设计并实现了统一对象系统与核心行为分派机制。通过将对象实体数据与类型行为分离为 `PyObject` 和 `Klass` 两个层次，并进一步引入 `KlassVtable` 组织对象核心行为分派，系统在保持 Python 语言动态性的同时，为高频核心操作提供了较稳定的内部调用路径。这种设计兼顾了属性字典查找、方法绑定、MRO 查找以及运行时核心操作分派，是本文对象系统部分较有代表性的工程创新。

第四，本文在轻量级 PVM 的实现中引入了句柄机制与 tracing GC 相配合的内存管理方案。相较于直接采用 CPython 以引用计数为主路径的对象管理方式，本文选择以追踪式 GC 为核心思路，并通过句柄槽位统一管理 native C++ 代码中持有的对象引用，从而使对象移动、根集合维护与解释器运行时状态管理能够被统一组织。这一方案既降低了轻量级系统实现循环引用处理等额外复杂度，也使系统在工程结构上更接近现代编程语言虚拟机的实现范式。

第五，本文在完成 VM 内核设计与实现的同时，还实现了面向宿主程序的 Embedder API，并形成了公共接口、桥接层与 VM 内核三层分界。这说明本文系统不仅关注“如何运行 Python 程序”，也关注“如何把该运行能力以脚本引擎方式提供给宿主程序使用”，从而进一步增强了本文工作的工程完整性与现实应用价值。

需要说明的是，作为本科毕业设计，本文的“创新性”更多体现为围绕特定目标完成系统级组织、关键机制取舍和真实工程落地，而非提出全新的编程语言理论或工业级性能优化算法。从本科毕业论文的评价标准来看，这种建立完整系统闭环、并通过测试验证其正确性与可用性的工作，已经具有较明确的研究价值与工程创新意义。

## 6.3 当前不足与未来的工作展望

尽管本文已经完成了 S.A.A.U.S.O VM 的核心功能闭环，并通过系统测试验证了其正确性与稳定性，但需要指出的是，编程语言虚拟机的设计与实现本身是一项高度复杂的工程。受课题定位、时间安排和工程规模控制等因素限制，现阶段系统仍然存在若干边界与不足，这些内容也构成了后续进一步完善的主要方向。

首先，在内存管理方面，当前系统已经实装新生代空间与基于 Cheney 算法的 Scavenge GC 主路径，但尚未完成完整的老生代回收、跨代引用维护以及工业级分代垃圾回收体系。换言之，现阶段的 GC 更偏向于支撑轻量级 PVM 形成最小可用闭环，而非追求工业级虚拟机在大规模对象存活场景下的全面优化。后续工作中，可以继续完善老生代空间、remembered set、写屏障等机制，使系统逐步演进为更完整的分代式 GC 架构。

其次，在 Python 语言功能覆盖方面，当前系统已经能够支持控制流、函数调用、闭包、对象系统、异常机制和模块导入等核心能力，但距离完整覆盖现代 Python 语言特性仍有明显差距。例如，descriptor 语义、更加完整的 `importlib` 机制、格式化字符串、生成器、协程以及更多内建类型和标准库模块等，当前仍未作为重点支持对象。后续可以在保持系统结构清晰的前提下，逐步扩展 Python 语言语义覆盖范围，并进一步提高与 CPython 的兼容程度。

再次，在前端编译链路方面，由于本文课题的重点是 PVM 后端，现阶段系统在源码到字节码的转换能力上可选复用 CPython 3.12 的成熟编译前端。这样的工程取舍有助于把研究重点集中在后端执行与运行时机制上，但也意味着当关闭该复用后，部分依赖源码编译前端的功能会受到限制。需要强调的是，这种边界体现的是本文系统在当前阶段的研究聚焦与工程取舍，而并不意味着 VM 后端本身无法独立运行。后续若条件允许，可以继续补强自有前端能力，或进一步完善 `.pyc` 装载与前端抽象层设计，使系统在输入链路上具备更强的独立性。

最后，在产品化与工程生态方面，当前系统已经具备较清晰的 Embedder API 分层与宿主互操作能力，但距离工业级脚本引擎产品仍有差距。例如，库产物组织方式、ABI 兼容策略、版本分发方式、宿主接入文档、性能基准体系以及跨平台发布流程等内容，目前尚未系统化建设。后续若以轻量级脚本引擎产品为目标，可以在现有 Embedder API 的基础上进一步明确这些工程问题，并逐步形成更加稳定的 SDK 形态。

总体而言，本文工作的重点在于建立一个兼容 CPython 3.12 核心字节码模型、结构清晰、具备教学与嵌入价值的轻量级 PVM 后端系统，而不是在毕业设计阶段追求对 Python 全语义、工业级性能优化和完整产品生态的全面覆盖。从当前实现结果来看，S.A.A.U.S.O VM 已经完成了对象系统、解释执行、异常传播、模块导入、内存管理与嵌入接口等关键能力的工程闭环，并通过测试验证了其可用性。未来若在语言覆盖范围、垃圾回收体系、编译前端独立性和产品化接口等方面继续演进，本文系统仍具有较大的扩展空间与研究价值。

# 参考文献

# 附录A S.A.A.U.S.O VM 的完整系统源码及开发文档

本文所实现的 S.A.A.U.S.O VM 系统的完整源代码、单元测试及相关开发文档已公开发布于 GitHub 平台，访问地址如下：
https://github.com/WU-SUNFLOWER/S.A.A.U.S.O
