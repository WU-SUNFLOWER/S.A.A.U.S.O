# 一个轻量级 Python 虚拟机的设计与实现

# 第1章 绪论

## 1.1 课题背景

Python 语言凭借其简洁优雅的语法、动态类型特性以及庞大的第三方生态系统，已成为当今Web开发、数据科学、人工智能及自动化运维等领域的首选语言。Python作为一种跨平台的、动态类型的高级编程语言，其执行依赖于Python虚拟机（Python Virtual Machine，PVM）。

虚拟机技术是现代高级编程语言的核心，它主要包括前端和后端两大部分。前端中由编译器（Compiler）将高级语言程序的源代码（Source Code）翻译成中间字节码（Bytecode）。后端中由解释器（Interpreter）负责执行字节码。此外，后端还提供垃圾回收器（Garbage Collector）和编程语言运行时（Runtime）等核心模块。这种机制使得Python或其他运用该项技术的高级编程语言，可以在任何安装了相应虚拟机软件的计算机系统上运行，而在源代码层面几乎不需要进行任何修改。

目前，官方标准的PVM实现——CPython，是基于C语言开发的，迄今已有 30 余年的历史。

虽然 CPython 具有极高的兼容性和稳定性，但其存在一些历史包袱和在设计上的局限性：
- **架构复杂且庞大**：CPython为了保证通用性，集成了大量的功能模块，导致其代码库庞大，对于仅需要轻量级脚本引擎功能的嵌入式场景（例如办公软件、游戏引擎等）而言，显得过于臃肿。
- **开发与维护难度大**：受限于 C 语言的表达能力，CPython 中的部分功能模块高度依赖手工管理。例如，CPython 主要依靠引用计数机制来实现自动内存管理，这就要求程序员手工维护 C 语言 native 层对 Python 对象的 reference count。这使得代码阅读和维护变得困难，不便进行深度的定制化修改。

基于上述背景，本课题提出使用现代 C++20 语言重新设计并实现一个可裁剪、易理解、可嵌入并且兼容标准 CPython 字节码的轻量级 PVM 后端，取名为 S.A.A.U.S.O VM。C++ 兼具 C 语言的底层控制能力，和现代高级语言的面向对象特性（如类、继承、模板、RAII 等），是开发现代工业级编程语言虚拟机的主流选择。利用 C++ 来实现 PVM，不仅可以保证虚拟机运行时性能的下限，更可以简化虚拟机的架构设计，使其代码更加清晰易读。

本课题所实现的 S.A.A.U.S.O VM，完整系统已公开发布在 GitHub 平台，详见附录 A。


## 1.2 国内外研究现状

### 1.2.1 国外研究现状

在Python语言实现与编程语言虚拟机领域，国外开源社区和科技企业的研究十分活跃。例如：
- PyPy：目前最著名的CPython替代方案。它利用JIT（即时编译）技术提升Python的运行速度，但其实现较重（内存占用较高，启动速度慢），不适合轻量级场景。
- MicroPython：针对嵌入式微控制器领域的Python实现。它极度精简，能够在只有16KB RAM 的设备上运行。然而，MicroPython为了极致的裁剪，牺牲了完整的 Python语言特性支持；且其核心仍基于C语言，扩展性受限。
- Codon：使用LLVM框架，直接将Python代码编译成原生的机器码直接执行，无需依赖PVM。但这意味着它不得不需要放弃Python的部分动态特性，例如动态类型的变量、在运行阶段可以被动态修改的类等。
- 对编程语言虚拟机的研究：Oracle公司 Strongtalk 虚拟机、Java HotSpot虚拟机与Google公司JavaScript V8虚拟机的研究成果在业界产生了深远影响。如内联缓存（Inline Caching）、Map对象模型（又称作Hidden Class）、分代垃圾回收等优化技术，已经成为许多工业级编程语言虚拟机的标配。

### 1.2.2 国内研究现状

国内在Python语言与编程语言虚拟机领域的研究相对起步较晚，并且小众。主要集中在以下几个方面：
- 应用层使用：大量的研究集中在使用Python作为编程语言，进行特定领域应用（如 Web后端、爬虫）的开发；或通过为CPython接入C扩展来加速特定模块，而非对PVM本身进行改造。
- 源码剖析与教学：近年来，随着Python语言的流行，越来越多的开发者和编程爱好者开始关注CPython内部实现，但大多数止步于源码阅读和简单的修改，能够从零构建完整PVM的开源项目非常少见。
- 对编程语言虚拟机的研究：国内的极少数互联网科技公司会围绕自己的业务需求，开发或定制适用于特定场景的编程语言虚拟机，如阿里巴巴推出面向的电商场景的 DragonwellJDK VM、字节跳动为Lynx跨端框架定制的PrimJS引擎等。但总体而言，这些实现往往闭源（或仅部分开源），缺乏广泛的开发者社区参与。

## 1.3 研究的内容及意义

由官方推出的 CPython 虚拟机历经 30 余年的迭代，已经高度成熟。诚然 S.A.A.U.S.O VM 的目的并不在取代 CPython，但它在教育教学、工程应用和人才培养方面，仍具有独特的实际价值。

### （1）教育教学价值

相较于 CPython 数十万行代码的体量，S.A.A.U.S.O VM 的核心代码只有 3 万多行，因此非常适合作为计算机相关专业本科《编译原理》《操作系统》等基础课程的授课教具或拓展资料，帮助学生直观理解高级编程语言的运行原理，具有一定的教学价值。

### （2）工程与实际应用价值

S.A.A.U.S.O VM 采用现代 C++20 语言进行开发，且实现了清晰的分层架构，十分便于开发人员对虚拟机进行裁剪、修改定制或二次开发，并适合作为轻量级脚本引擎嵌入到办公软件、游戏引擎等宿主应用，具有一定的工程与实际应用价值。

### （3）人才培养价值

S.A.A.U.S.O VM 的设计与实现，不同于常见的 Web 前后端系统本科毕设，涉及语言运行时、对象系统、内存管理、异常机制、模块加载等底层核心问题，要求综合调动多门本科计算机专业核心课程的知识，可以充分锻炼毕业学生的复杂系统设计与工程落地实现的完整能力。



# 第2章 Python 语言的核心功能与 PVM 的执行原理

本章将介绍 Python 的核心语言功能，以及实现 CPython 3.12 字节码规范的 PVM 的基本执行原理。

本章首先将介绍 PVM 作为一种虚拟的堆栈机器的基本原理，再逐一介绍 Python 语言中常用的核心语言功能，以及它们在 PVM 内部的基本实现原理。

本章是后续自行设计并实现一个兼容标准 CPython 字节码的轻量级 PVM 的理论基础。

## 2.1 PVM 执行原理综述

Python 语言的执行模型主要由编译器前端和 PVM 后端两部分组成。

Python 编译器前端首先将 Python 源代码解析成 AST 树，再通过后序遍历的方式，进一步翻译成二进制的、针对栈机的字节码文件（.pyc文件）。

构成 Python 字节码文件的基本数据结构一般被称作 CodeObject，它主要由以下几个部分组成：
- 字节码指令序列（Sequence of bytecodes）：由一段连续的字节码指令组成的完整程序。类似于面向 CPU 硬件的汇编指令，在 CPython 3.12 的字节码规范中，每一条 Python 字节码指令由操作码（operation code）和操作参数（operation parameter）两部分组成。每一条字节码指令的长度均为 2 个字节，其中操作码和操作参数各占一个字节。
- 符号列表（Names list）：用于存储变量名、函数名等符号的字符串列表。
- 常量列表（Constants list）：用于存储常量值（如数值字面量、字符串字面量等）的字符串列表。

PVM 的运行过程类似于计算机的 CPU，会逐条读取字节码指令，并进行解释执行。
  - PVM 在执行过程中，会维护一个操作数栈。
  - 一条字节码在被解释执行时，PVM 首先会从操作数栈中弹出若干个元素，并按照对应字节码的规则进行运算或处理，再将所得结果（即这条字节码的执行结果）压回栈顶。
  - 当所有字节码指令都被解释执行完毕后，PVM 操作数栈中的栈顶元素，即为程序的最终执行结果。

下面通过一个简单的例子来进一步说明 PVM 的运行过程：
```python
x = 1
y = 2
z = 3
result = x + y * z
```

首先，CPython 3.12 编译器前端（下文均简称"编译器"，不再重复强调具体的版本号）所生成 CodeObject 的主要内容如下：

```python
Sequence of bytecodes:
LOAD_CONST               0 (1)
STORE_NAME               0 (x)

LOAD_CONST               1 (2)
STORE_NAME               1 (y)

LOAD_CONST               2 (3)
STORE_NAME               2 (z)

LOAD_NAME                0 (x)
LOAD_NAME                1 (y)
LOAD_NAME                2 (z)
BINARY_OP                5 (*)
BINARY_OP                0 (+)

Names list:
['x', 'y', 'z']

Constants list:
[1, 2, 3]
```

当 PVM 启动后，会根据 CodeObject 中的字节码指令序列，逐条解释执行。

首先，PVM 会取出第一条指令 `LOAD_CONST 0`，这表示取出常量列表中的第一个元素 `1`，并将其压入操作数栈。
然后，PVM 会取出第二条指令 `STORE_NAME 0`，这表示将操作数栈中的栈顶元素（即 `1`）弹栈并赋值给符号列表中下表为 0 的元素 `x`。
接下来，与之类似，PVM 会完成对变量 `y` 和 `z` 的赋值。

接下来的三条 `LOAD_NAME` 指令，分别表示读取变量 `x`（对应名称列表中下标为 0 的变量名）、变量 `y`（对应名称列表中下标为 1 的变量名）和变量 `z`（对应名称列表中下标为 2 的变量名），并将其压入操作数栈。这三条指令执行完毕后，PVM 操作数栈从栈顶到栈底的元素分别为 `3`、`2` 和 `1`。

接下来的两条 `BINARY_OP` 指令分别表示一个二元操作。第一条 `BINARY_OP` 指令的操作参数为 `5`，对应的操作为乘法计算。PVM 执行这条指令时，首先会弹出栈顶处的两个元素（即 `3` 和 `2`），并执行乘法运算，再将结果重新压回操作数栈的栈顶。执行结束后，PVM 操作数栈从栈顶到栈底的元素分别为 `6` 和 `1`。

最后一条 `BINARY_OP` 指令的操作参数为 `0`，对应的操作为加法计算。与上一条指令类似，PVM 会弹出操作数栈中的 `6` 和 `1`，并将计算结果 `7` 压回栈顶。现在操作数栈中只剩一个 `7` 了，它就是上述 Python 代码的最终执行结果。

## 2.2 Python 中基本的控制流

与 CPU 硬件类似，PVM 中也会维护一个程序计数器（program counter），用于表示下一条将要执行指令的地址（在 PVM 中，实际上指的就是指令在字节码指令序列中的下标）。

在此基础上，Python 便引入了一系列通过修改 PVM 内部程序计数器来实现控制流跳转的字节码指令，例如 `JUMP_IF_TRUE`、`JUMP_IF_FALSE`、 `JUMP_FORWARD` 等。与汇编语言类似，Python 语言中的循环、条件等控制流，就是基于这些跳转指令实现的。

例如下面是一个最简单的例子，其中 `condition` 表示一个循环条件：
```python
while condition:
    i += 1
print(i)
```

编译器生成的字节码指令序列如下：
```python
2 LOAD_NAME                0 (condition)
4 POP_JUMP_IF_FALSE        6 (to 18)
6 LOAD_NAME                1 (i)
8 LOAD_CONST               0 (1)
10 BINARY_OP               13 (+=)
14 STORE_NAME               1 (i)
16 JUMP_BACKWARD            8 (to 2)
18 后续代码略...
```

其中 `POP_JUMP_IF_FALSE` 字节码指令表示"将栈顶元素弹出，如果它可以被转换为 False，则执行跳转"。当 PVM 执行这条指令时，程序计数器 IP 的值为 6（即下一条指令的地址），在 CPython 3.12 的字节码规范中，该指令的目标跳转地址计算公式为 `IP + 操作参数 * 2`；因此当栈顶元素可以被转换为 False 时，PVM 会跳转到地址为 18 位置的指令继续执行，这就实现了"循环条件不成立时跳过循环体"的效果。

`JUMP_BACKWARD` 字节码指令表示"无条件向前跳转"。它的目标跳转地址计算公式为 `IP - 操作参数 * 2`，因此执行到它时，PVM 会跳转到地址为 2 位置的指令继续执行，这就实现了"循环体执行结束后重新检测循环条件是否成立"的效果。

## 2.3 Python 中的函数

### 2.3.1 简单函数的创建

前文提到，CodeObject 是 Python 字节码文件的基本组成单位。实际上，对于单个 Python 程序中的每个函数，实际上 Python 编译器都会为它们分别生成一个独立的 CodeObject，其中分别封装了每个函数自己的字节码指令序列、符号列表和常量列表等数据。

特别地，虽然 Python 语言中没有类似于 C/C++ 等语言的 `main` 函数，但实际上编译器仍然会将最外层的代码视作一个函数。这样便统一了 CodeObject 的语义，更加方便 PVM 的实现。

不同于 C/C++ 等静态语言，尽管 Python 中的 CodeObject 是在编译阶段确定的，但函数作为一种特殊的 Python 对象，是在运行时由 PVM 通过执行 `MAKE_FUNCTION` 字节码动态创建的。下面通过一个具体的例子进行说明。

对于下面这个例子：
```python
def add(a, b):
    return a + b

print(1, 2)
```

编译器生成的字节码文件的主要内容如下：

```python
 2 LOAD_CONST               0 (<code object add at 0x58dd4e7d1ca0, file "example.py", line 1>)
 4 MAKE_FUNCTION            0
 6 STORE_NAME               0 (add)
 8 后续代码略...

Disassembly of <code object add at 0x58dd4e7d1ca0, file "example.py", line 1>:
 2 LOAD_FAST                0 (a)
 4 LOAD_FAST                1 (b)
 6 BINARY_OP                0 (+)
10 RETURN_VALUE
```

从中我们可以观察到最外层代码对应 CodeObject 的常量列表中，下标为 0 的元素即为 Python 函数 `add` 所对应的 CodeObject。

首先，PVM 会执行 `LOAD_CONST` 指令加载这个 CodeObject 到栈顶，再由 `MAKE_FUNCTION` 字节码进行消费。接下来，`MAKE_FUNCTION` 字节码会将 CodeObject 包装为一个真正的 Python 函数对象并压回栈顶。最后，`STORE_NAME` 字节码会将这个函数对象保存为名为 `add` 的变量。这样，一个 Python 函数对象的创建过程就完成了。

### 2.3.2 栈帧与函数调用的原理

在 Python 中，每当执行函数调用时，PVM 都会创建一个新的栈帧（Stack Frame）。

栈帧中维护了当前函数的操作数栈、局部变量表、程序计数器等关键状态与数据。

当前函数执行结束时，当前栈帧的操作数栈上的栈顶元素即为函数的返回值。PVM 会将返回值取出并暂存并销毁当前栈帧，再将返回值压入调用方栈帧操作数栈，最后继续向下执行调用方函数。

### 2.3.3 扩展参数与键值对参数

在 Python 中，当调用方传入的实参个数大于被调用函数的形参个数时，多余的参数会被 PVM 打包成一个 list 传递给被调函数；被调函数可以通过在形参列表中声明 `*args` 的方式接收该 list。

当调用方传入多余的键值对参数时，这些参数会被 PVM 打包成一个 dict 传递给被调函数；被调函数可以通过在形参列表中声明 `**kwargs` 的方式接收该 dict。

### 2.3.4 函数闭包

在 Python 中

### 2.3.5 常用的内建函数（Builtin Function）

- print
- len
- isinstance

## 2.4 Python 中的面向对象机制

### 2.4.1 综述

- Python 中既包括一系列内建类型，又允许用户自定义类
- 与多数面向对象编程语言类似，面向对象的三大特性（封装、继承、多态）在 Python 语言中均有体现。

### 2.4.1 基本内建类型

- object
- int
- float
- str
- list
- dict

### 2.4.3 类的封装

封装指将数据（属性）和操作数据的方法捆绑在一起，并隐藏内部实现细节，只对外暴露必要的接口。

与大多数面向对象的编程语言类似，Python 允许用户为自定义类添加方法与属性，从而实现数据驱动方法执行。

但不同于 C++、Java 等其他的主流编程语言，Python 语言出于动态性和灵活性的考虑，并不支持用户设置类的方法与属性的可见性（例如 public、protected、private 等）。

### 2.4.4 类的继承

与大多数面向对象的编程语言类似，Python 继承允许一个类（子类）基于另一个类（父类）来定义，从而复用代码并建立层次关系。

```python
class Animal:
    def __init__(self, name):
        self.name = name

    def speak(self):
        return "Some sound"

class Dog(Animal):                     # 单继承
    def speak(self):                   # 重写 speak
        return f"{self.name} says Woof!"

    def wag_tail(self):
        return "Tail wagging"

class Cat(Animal):
    def speak(self):
        return f"{self.name} says Meow!"

dog = Dog("Buddy")
print(dog.speak())      # Buddy says Woof!
print(dog.wag_tail())   # Tail wagging

cat = Cat("Kitty")
print(cat.speak())      # Kitty says Meow!
```

特别地，Python 是一门支持多继承的编程语言。一个子类可以继承多个父类，class C(A, B):。

为了确定再多个父类存在同名方法的情况下，子类应当选取哪个父类的方法进行继承，Python 中引入了 MRO 序列（Method Resolution Order）的概念。

当子类创建时，PVM 会执行 C3 线性化算法对该类包括父类在内的所有祖先类进行排序，所得结果即为 MRO 序列。

当子类尝试调用继承所得的方法时，PVM 会遍历 MRO 序列中的祖先类，并尝试在祖先类中查找目标方法。第一个找到的目标方法即为结果。

### 2.4.5 类的多态

多态指同一个接口（方法名）在不同对象上表现出不同的行为。Python 是动态类型语言，多态体现得更加自然，主要依赖鸭子类型和方法重写。
- 方法重写：子类重写父类方法后，调用相同方法名会执行子类版本。
- 鸭子类型：不关心对象的具体类型，只关心它是否具有所需的方法或属性。“如果它走起来像鸭子、叫起来像鸭子，那么它就是鸭子。” 这使得 Python 的多态不依赖于继承体系，只要对象实现了相应方法，就可以被统一处理。

```python
# 多态示例：不同对象调用相同接口
def make_sound(animal):      # 函数不限定参数类型
    print(animal.speak())    # 只要 animal 有 speak() 方法即可

make_sound(Dog("Rex"))       # Rex says Woof!
make_sound(Cat("Lucy"))      # Lucy says Meow!
make_sound(Animal("Unknown"))# Some sound

# 鸭子类型示例
class Robot:
    def speak(self):         # 没有继承 Animal，但实现了 speak
        return "Beep boop"

make_sound(Robot())          # Beep boop —— 也能工作

# 另一个例子：len() 函数的多态
print(len("hello"))          # 字符串：5
print(len([1, 2, 3]))        # 列表：3
print(len({"a": 1}))         # 字典：1
# 这些类型都实现了 __len__ 方法，所以 len() 可以统一处理
```

## 2.5 Python 中的异常机制

## 2.6 Python 中的模块机制

# 第3章 S.A.A.U.S.O VM 的设计与实现

# 第4章 S.A.A.U.S.O VM 的测试与结论

# 第5章 S.A.A.U.S.O VM 的能力边界与未来展望

# 参考文献

# 附录1 S.A.A.U.S.O VM 的完整系统源码及开发文档

本文所实现的 S.A.A.U.S.O VM 系统的完整源代码、单元测试及相关开发文档已公开发布于 GitHub 平台，访问地址如下：
https://github.com/WU-SUNFLOWER/S.A.A.U.S.O
