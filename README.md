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


## 3. 本项目支持的 Python 能力

### 3.1 概述

本项目支持的 Python 语言核心功能子集包括：
- 控制流：`if/while/break/continue`
- 内建类型：`list/dict/tuple/str/bool/None`
- 函数机制：位置参数、关键字参数、默认值、`*args/**kwargs`、闭包变量
- 类型系统：自定义类、类的继承、方法绑定、魔法方法
- 导入系统：模块、包、相对导入、内建模块
- 异常系统：抛出异常、异常传播、异常捕获、`finally`

### 3.2 典型例子

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


## 4. 如何把虚拟机嵌入到你的应用程序

### 4.1 核心概念介绍

S.A.A.U.S.O 提供一套面向宿主程序的 Embedder API。  

这套 API 中包含几个关键概念：
- **`Isolate`**：代表一个 VM 实例。
  - 在宿主程序中，必须先"进入"一个具体的 Isolate 实例，之后才能使用 VM 的功能。
  - 使用 VM 功能结束后，需要"退出"该 Isolate 实例。
- **`Local<T>`**：类似于 C++ 中智能指针的概念，在 S.A.A.U.S.O 中必须使用它来持有被分配在 VM 堆上的对象。
- **`HandleScope`**：用于控制 C++ 代码中 `Local<T>` 的生命周期。
  - 当一个 `HandleScope` 被析构时，在它所处作用域内的所有 `Local<T>` 对 VM 堆上对象的引用都会失效。
  - 在根 `HandleScope` 被创建前，不得在 VM 堆上创建对象。
- **`Context`**：分配在 VM 堆上，代表一个 Python 脚本执行的全局环境。
- **`Script`**：分配在 VM 堆上，代表一个可执行的 Python 脚本。
  - 运行 Python 脚本时，需要提供一个有效的 `Context`。

### 4.2 一个最简单的例子

要将 S.A.A.U.S.O 接入宿主程序，一个最基本的流程如下：
1. 初始化 S.A.A.U.S.O 库（调用 `Saauso::Initialize()`）。
1. 创建一个虚拟机单例（调用 `Isolate::New()`）
1. 进入该虚拟机单例。
1. 创建一个根 `HandleScope` 实例。
1. 创建一个默认的 `Context` 实例（调用 `Context::New()`，并使用 `Local<Context>` 接住）。
1. 创建一个 `Script` 并编译（调用 `Script::Compile()`，并使用 `Local<Script>` 接住）。
1. 运行脚本（调用 `Script::Run(context)`）。
1. 退出虚拟机单例。
1. 销毁虚拟机单例。
1. 关闭 S.A.A.U.S.O 库（调用 `Saauso::Dispose()`）。

一个具体的例子（执行 Python 脚本 HelloWorld）如下：
```C++
int main() {
  // 1. 初始化 S.A.A.U.S.O 库
  saauso::Saauso::Initialize();
  // 2. 创建虚拟机实例
  saauso::Isolate* isolate = saauso::Isolate::New();

  {
    // 3. 创建一个与 isolate 绑定的 scope，之后会自动进入 isolate 实例 
    saauso::Isolate::Scope isolate_scope(isolate);

    // 4. 创建一个 HandleScope
    saauso::HandleScope scope(isolate);

    // 5. 创建一个默认的全局环境
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);

    // 6. 创建并编译一段Python脚本
    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "print('Hello World')\n"));

    // 7. 运行编译好的Python脚本
    maybe_script.ToLocalChecked()->Run(context);

    // 8. 此处 isolate_scope 会被析构，然后 isolate 会自动退出
  }

  // 销毁虚拟机实例
  isolate->Dispose();
  // 关闭 S.A.A.U.S.O 库
  saauso::Saauso::Dispose();
  return 0;
}
```

完整代码，见[samples/hello-world.cc](./samples/hello-world.cc)

### 4.3 注入 C++ 函数供 Python 脚本调用

在许多实际的业务场景中，嵌入方需要将一个 C++ 函数注入进 Python 世界的全局环境，以便于脚本进行调用。下面通过一个实际的例子，来说明如何在 S.A.A.U.S.O 中实现这一需求。

假设我们有如下的 Python 脚本，其中 `to_binary_string` 是宿主要注入的 C++ 函数，它的功能是将一个整型转为对应的二进制字符串。

```python
value = 2026
result = to_binary_string(value)
print(result) # 预期输出字符串"11111101010"
```

下面分两步来介绍如何实现这个效果。

#### 4.3.1 实现宿主侧的 C++ 函数

在编写代码之前，你肯定会问，我们应该如何在宿主的 C++ 函数中，拿到 Python 脚本执行函数调用的上下文信息（例如 Python 脚本传入的实参、发起函数调用的虚拟机实例）呢？

答案是，S.A.A.U.S.O 为嵌入方提供了一个名为 `FunctionCallbackInfo` 的抽象数据结构，其中就封装了这些必要的上下文信息。并且它的使用非常简单。

首先我们先定义这个准备注入的 C++ 函数，并且将 `FunctionCallbackInfo` 作为它唯一的形参：
```C++
using namespace saauso;

void ToBinaryString(saauso::FunctionCallbackInfo& info) {}
```

然后，我们直接通过访问 `info[0]` 将 Python 脚本传入的整型数据取出来：
```C++
int64_t value;
if (!info[0]->ToInteger().To(&value)) {
  // 如果 info[0] 不存在，或者它不是整型类型的，那么抛出异常
  info.ThrowRuntimeError("to_binary_string except one int argument.");
  return;
}
```

取到 Python 脚本传入的参数后，我们直接编写"整型转二进制字符串"的代码即可：
```C++
std::string result;
if (value != 0) {
  while (value > 0) {
    result = static_cast<char>(value % 2 + '0') + result;
    value /= 2;
  }
} else {
  result = "0";
}
```

接下来，由于我们现在得到的是一个 `std::string`，因此我们要先将它转换成 Python 世界对应的字符串对象。这可以通过 `String::New` API 实现：
```C++
Isolate* isolate = info.GetIsolate();
Local<String> result_str = String::New(isolate, result);
```

> 为什么 `String::New` 的第一个参数是 `Isolate*` 呢？因为 S.A.A.U.S.O 中不同 VM 实例的堆是互相隔离的；在 Python 世界创建字符串时，它需要明确地知道需要在哪个 VM 实例的堆上进行创建！

最后，我们通过 `FunctionCallbackInfo` 提供的 `SetReturnValue` 方法，将 `result_str` 作为函数的返回值"写回" Python 世界：
```C++
info.SetReturnValue(result_str);
```

到此为止，宿主侧要注入的 C++ 函数 `ToBinaryString` 就编写完成了。

#### 4.3.2 将宿主侧的 C++ 函数注入进 Python 世界的全局环境

在 S.A.A.U.S.O 中，宿主只需将需要注入的全局变量/函数，写进脚本执行时所提供的 `Context` 实例中。Python 脚本在运行时，会将宿主提供的 `Context` 中的内容自动作为全局环境使用。

首先，同之前的 Hello World 例子，我们需要创建一个 `Context` 实例：
```C++
Local<Context> context = Context::New(isolate);
```

然后，我们需要将刚才实现的 `ToBinaryString` C++ 函数包装成一个 Python 世界的 `Function` 对象：
```C++
constexpr std::string_view kInjectedFuncName = "to_binary_string";
Local<Function> injected_func = 
    Function::New(isolate, &ToBinaryString, kInjectedFuncName);
```

接下来，我们将要注入的函数名作为"键"、 `Function` 对象作为"值"，注入进 `context` 中：

```C++
// 要注入的函数名，一样要先转成 Python 对象
Local<String> injected_func_name = String::New(isolate, kInjectedFuncName);

// 将要注入的函数名作为"键"、 Function 对象作为"值"，注入进 context
Maybe<void> set_result = context->Global()->Set(injected_func_name, injected_func);

// 理论上操作不可能失败，这里的报错不可能触发！
if (set_result.IsNothing()) [[unlikely]] {
  std::cerr << "set global failed" << std::endl;
  return 1;
}
```

最后，我们使用这个已经注入了 C++ 函数的 `context`，来运行 Python 脚本，即可看到效果：
```C++
// 创建并编译一段Python脚本
MaybeLocal<Script> maybe_script =
    Script::Compile(isolate, String::New(isolate, kPythonScript));
// 运行编译好的Python脚本
maybe_script.ToLocalChecked()->Run(context);
```

完整代码，见[samples/inject-cpp-func-to-python-world.cc](./samples/inject-cpp-func-to-python-world.cc)

### 4.4 在宿主 C++ 程序中调用 Python 函数

在另外的一些业务场景中，宿主程序需要主动调用脚本中的 Python 函数。下面仍然通过一个实际的例子，来说明如何在 S.A.A.U.S.O 中实现这一需求。

假设有一段 Python 脚本，会在全局环境中创建如下函数：
```python
def to_binary_string(value):
  result = ""
  
  if value == 0:
    return "0"
  
  while value > 0:
    result = str(value % 2) + result
    value //= 2
  return result
```

我们已经知道，在 S.A.A.U.S.O 中，Python 脚本的全局环境，即对应于宿主 C++ 程序中的 `Context` 实例。

这意味着，一旦这段脚本执行后，宿主程序可以直接通过 `Context` 实例拿到 `to_binary_string` 这个 Python 函数在 C++ 世界对应的 `Function` 对象。

代码如下：
```C++
// 要获取的全局函数名称。
constexpr std::string_view kPythonFuncName = "to_binary_string";
Local<String> func_name = String::New(isolate, kPythonFuncName);

// 以 func_name 为"键"，从 context 中拿到对应的"值"，
// 注意，这里的返回值的类型为基类 Value。
Local<Value> raw_func = 
    context->Global()->Get(func_name).ToLocalChecked();

// 将 raw_func 转化为可以被宿主调用的 Function 类型。
auto func = Local<Function>::Cast(raw_func);
```

然后，我们直接设置实参、调用该 Function 对象。在 S.A.A.U.S.O 内部，该 Function 对象对应的 Python 函数会被 VM 执行：
```C++
Local<Value> argv[] = {Integer::New(isolate, 2026)};
MaybeLocal<Value> result = func->Call(context, Local<Value>(), 1, argv);
```

最后，我们可以将返回结果转换成 `std::string`，并打印到终端：
```C++
Maybe<std::string> result_in_std = result.ToLocalChecked()->ToString();
std::cout << result_in_std.ToChecked() << std::endl;
```

完整代码，见[samples/call-python-func-in-cpp.cc](./samples/call-python-func-in-cpp.cc)

### 4.5 文档索引

如果你需要更详细的接入文档，请阅读：
- [Embedder API 接入指南](./docs/Saauso-Embedder-API-User-Guide.md)

如果你想要了解 Eembedder API 的架构设计思路，请阅读：
- [Embedder API 架构设计](./docs/architecture/Embedder-API-Architecture-Design.md)

## 5. 本项目的整体架构

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

## 6. 本项目的亮点与创新点

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
- 借鉴 V8 的 `Embedder API` 思路，为嵌入方提供 VM 功能的接入方案。

## 7. 当前边界与未来展望

编程语言 VM 开发是一项高度复杂的工程。为了保证系统验收阶段的稳定性，当前版本重点聚焦于 Python VM 后端的核心功能闭环，而非在答辩前继续扩展功能面。

本项目当前的阶段性边界与未来演进方向，包括：
- 当前 VM 中已经实装 Scavenge GC 算法
  - **未来展望**：后续可以继续扩展为类似工业级 VM 的分代式 GC 架构。
- 当前 VM 已经支持 Python 语言的核心功能子集
  - **未来展望**：后续可以对支持的 Python 语言功能进行进一步扩展，例如 format string、descriptor、生成器与协程等。
- 当前 Embedder API 已具备清晰的公共接口与内部实现分层
  - **未来展望**：后续可明确库产物、ABI 策略与分发策略等事宜，并推出产品化的 VM SDK。
- 由于本项目的研究课题为 PVM 后端，当前源码到字节码的前端编译链路可选复用 CPython 3.12 的成熟能力；**关闭该复用后，VM 后端仍可独立运行**，但部分依赖源码编译前端的功能会受到限制。

> 上述内容体现的是本项目当前阶段的工程边界、工程取舍与未来演进方向，而非系统无法验收或能力闭环缺失。当前系统已经具备对象系统、解释执行、内存管理等 PVM 核心机制并形成闭环，即支撑系统验收演示所需的核心能力。

## 8. 项目的构建与测试

### 8.1 环境准备

Windows 系统，请自行安装并配置以下工具：
- **Visual Studio Build Tools**：用于提供 Windows 环境下的链接器支持
- **LLVM Clang**：核心编译环境，VS Code 中的 clangd 插件也依赖该环境
- **MSYS2**：用于进一步安装 UCRT 环境，以便于构建 Asan/UBSan 包
- **Git**：版本管理工具，以及运行`build.sh`所需的 Git Bash 终端

Linux 系统，直接运行以下脚本：
```shell
./tools/llvm_setup_for_linux.sh
```

### 8.2 构建命令

构建单元测试（自动开启 Asan/UBSan）：
```shell
./build.sh ut
```

构建 Embedder API Demo：
```shell
./build.sh demo
```

### 8.3 如何跑单元测试

跑 VM 核心单元测试：
```shell
./out/ut/ut
```

跑 Embedder API 单元测试：
```shell
./out/ut/embedder_ut
```

更多细节，请参考[docs/Saauso-Build-and-Test-Guide.md](./docs/Saauso-Build-and-Test-Guide.md)
