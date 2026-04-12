# S.A.A.U.S.O Embedder API 接入指南（MVP）

## 1. 这是什么

S.A.A.U.S.O Embedder API 用于把脚本能力嵌入到 C++ 宿主程序中。  
你可以：

1. 在 C++ 创建 `Isolate/Context`
2. 向脚本侧注入宿主回调（`Function::New`）
3. 编译运行脚本（`Script::Compile/Run`）
4. 用 `TryCatch` 捕获错误
5. 在 C++ 与脚本间交换 `String/Integer/Float/Boolean/Object/List/Tuple`

## 1.1 零基础先懂这些概念

如果你把“脚本引擎”看成一个小型可编程系统，可以这样理解：

1. `Isolate`：一个独立“脚本世界”
   - 它是最外层运行容器，里面有自己的对象、异常状态和执行上下文。
   - 一个 `Isolate` 崩溃或抛错，不会自动影响另一个 `Isolate`。
   - 实践中通常是“每个业务沙箱/插件系统一个 Isolate”。

2. `Context`：这个世界里的“当前工作环境”
   - 你可以把它理解成一张全局变量表（globals）+ 当前执行语境。
   - `Context::Set/Get` 是快捷读写；`Context::Global()` 返回 `Object` 让你做更丰富操作（如 `CallMethod`）。
   - `Context::New` 每次都会创建一个独立 globals，不同 Context 之间默认隔离。
   - 脚本里写的全局函数/变量，本质都挂在当前运行的这个环境上。

3. `Script`：已编译的脚本对象
   - `Script::Compile` 会把字符串源码编译成一个可重复执行的脚本对象。
   - `Run` 只负责执行，不会在内部再次重新编译源码。
   - 当前实现里，脚本执行时会把传入 `Context` 的底层字典同时作为根栈帧的 `globals` 与 `locals` 使用。
   - 任何一步失败都可能返回空 `MaybeLocal`，并由 `TryCatch` 捕获错误。

4. `Local<T>`：一个“受作用域管理”的安全引用
   - 它不是裸指针，而是 API 层的句柄包装。
   - `Local<T>` 的生命周期受 `HandleScope` 管理，离开作用域后不应继续使用。
   - 这样做是为了防止内存泄漏和悬挂引用。

5. `MaybeLocal<T>`：可能有值，也可能失败
   - 成功时可 `ToLocal` 取出结果。
   - 失败时是空值（`IsEmpty()==true`），必须配合 `TryCatch` 看具体异常。
   - 这是 Embedder API 里最重要的防御式调用约定。

6. `Isolate::Scope`：Isolate 绑定的 RAII 守卫
   - 构造时调用 `Enter()`，析构时调用 `Exit()`。
   - 它负责把“当前线程”与目标 `Isolate` 绑定。
   - 所有带 `Isolate*` 的 Embedder API 调用都应在该作用域内进行。

7. `Locker`：跨线程共享同一 Isolate 时的串行化守卫
   - 构造时加锁，析构时解锁。
   - 支持同线程重入。
   - 加锁与解锁必须发生在同一线程，违约会直接终止进程。

8. `HandleScope`：一段“临时对象自动回收区间”
   - 在这段区间创建的包装对象会被统一管理。
   - 区间结束自动回收，避免长期累积导致内存膨胀。
   - `HandleScope` 只负责句柄生命周期，不包含其他隐式的语义。
   - `EscapableHandleScope` 则允许你把一个返回值“逃逸”到外层继续使用。

9. `TryCatch`：错误观察器
   - 只要你要调用可能失败的 API（编译、运行、方法调用），就建议先创建 `TryCatch`。
   - 调用后检查 `HasCaught()`，这是判定“脚本侧是否抛错”的标准方式。

一句话串起来：  
`Isolate` 提供运行沙箱，`Isolate::Scope` 绑定线程上下文（跨线程共享时叠加 `Locker`），`Context` 提供全局环境，`Script` 提供可执行逻辑，`Local/MaybeLocal` 提供安全结果传递，`HandleScope/TryCatch` 提供内存与异常保障。

## 2. 通过几个案例快速上手

下面让我们通过几个实际的例子，把上面的这些概念串起来，搞清楚具体如何将脚本引擎嵌入进宿主程序。

### 2.1 跑通 Hello World

要将 S.A.A.U.S.O 接入宿主程序，一个最基本的流程如下：
1. 初始化 S.A.A.U.S.O 库（调用 `Saauso::Initialize()`）。
1. 创建一个虚拟机单例（调用 `Isolate::New()`）
1. 进入该虚拟机单例。
1. 创建一个根 `HandleScope` 实例。
1. 创建一个默认的 `Context` 实例（调用 `Context::New()`，并使用 `Local<Context>` 接住）。
1. 创建一个 `Script` 并编译（调用 `Script::Compile()`，并使用 `MaybeLocal<Script>` 接住）。
1. 运行脚本（调用 `Script::Run(context)`；运行时会直接复用该 `Context` 作为脚本的全局环境）。
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

    // 7. 运行编译好的 Python 脚本
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

### 2.2 注入 C++ 函数供 Python 脚本调用

在许多实际的业务场景中，嵌入方需要将一个 C++ 函数注入进 Python 世界的全局环境，以便于脚本进行调用。下面通过一个实际的例子，来说明如何在 S.A.A.U.S.O 中实现这一需求。

假设我们有如下的 Python 脚本，其中 `to_binary_string` 是宿主要注入的 C++ 函数，它的功能是将一个整型转为对应的二进制字符串。

```python
value = 2026
result = to_binary_string(value)
print(result) # 预期输出字符串"11111101010"
```

下面分两步来介绍如何实现这个效果。

#### 2.2.1 实现宿主侧的 C++ 函数

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

#### 2.2.2 将宿主侧的 C++ 函数注入进 Python 世界的全局环境

在 S.A.A.U.S.O 中，宿主只需将需要注入的全局变量/函数，写进脚本执行时所提供的 `Context` 实例中。当前 `Script::Run(context)` 的实现会直接复用这个 `Context` 底层字典作为根栈帧的全局环境；因此脚本顶层定义的变量与函数，执行完成后也会继续保留在这个 `Context` 里。

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

### 2.3 在宿主 C++ 程序中调用 Python 函数

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

## 3. 常用 API 速览

### 3.1 基础生命周期

1. `Isolate::New/Dispose`
2. `Isolate::Scope`（推荐优先使用 RAII 管理 `Enter/Exit`）
3. `HandleScope`
4. `Context::New/Enter/Exit`
5. `Context::Global`（返回全局字典对应的 `Object`）

### 3.2 Context 适用场景与语义

1. `Context::New` 语义
   - 每次调用都创建一个新的执行环境（独立 globals）。
   - 适用于“同一 Isolate 下多业务域隔离”（如房间、插件、会话、脚本租户）。

2. `Enter/Exit` 语义
   - 每个 Isolate 内维护 Context 栈，`Enter` 入栈，`Exit` 只允许退出栈顶 Context（LIFO）。
   - `Context::Scope` 是 RAII 版本：构造时 `Enter`，析构时 `Exit`。
   - 建议优先使用 `Context::Scope`，避免手动配对错误。

3. 隔离效果
   - 变量隔离：`context_a` 的全局变量不会出现在 `context_b`。
   - 回调绑定隔离：只注入到 `context_a` 的宿主函数不会自动出现在 `context_b`。
   - 异常隔离：某个 Context 中的脚本失败不会污染另一个 Context 的后续执行路径。

### 3.3 Script 适用场景与语义

1. `Script::Compile` 语义
   - 输入是一段源码字符串，输出是一个已编译的脚本对象。
   - 编译阶段依赖 `SAAUSO_ENABLE_CPYTHON_COMPILER`；未开启时，`Compile` 失败是预期行为。
   - 同一个 `Script` 可以在后续被重复执行。

2. `Script::Run(context)` 语义
   - `Run` 不会再次重新编译源码，而是直接执行 `Compile` 阶段得到的脚本对象。
   - 当前实现中，`context` 底层字典会同时作为根栈帧的 `globals` 与 `locals`。
   - 这意味着脚本顶层写入的名字会直接保留在 `context` 中，宿主可在运行后通过 `Context::Get` 或 `Context::Global()` 继续访问。

3. 当前 MVP 注意事项
   - 当前不会额外向 `globals` 自动镜像 `__builtins__`。
   - 当前也没有为 `Script::Run` 统一自动注入 `__name__`。
   - 如果宿主脚本依赖 `__name__` 等名字，请嵌入方先显式写入对应 `Context`。

### 3.4 值类型

1. `String` / `Integer` / `Float` / `Boolean`
2. `Object`：`Set/Get/CallMethod`
3. `List`：`Length/Push/Set/Get`
4. `Tuple`：`New/Length/Get`

### 3.5 回调与异常

1. `Function::New`（注册宿主回调）
2. `Function::Call`
3. `FunctionCallbackInfo`（读参数、写返回值、`ThrowRuntimeError`）
4. `TryCatch`（统一捕获 API 失败）
5. `Exception::TypeError/RuntimeError`（MVP 工厂接口）
6. `Isolate::ThrowException`（宿主主动向当前传播域注入异常）

## 4. 错误处理规范（必须遵守）

1. 所有可失败 API 都可能返回空 `MaybeLocal<T>`。
2. 只要你关心失败原因，就在调用前创建 `TryCatch`。
3. `MaybeLocal` 失败 + `TryCatch.HasCaught()==true` 才是完整错误闭环。
4. 不要忽略空句柄；必须先判空再 `ToLocalChecked`。
5. `TryCatch::Exception()` 返回 `Local<Value>`；未捕获异常时返回空句柄。
6. 对于 `Script::Compile/Run`、`Function::Call`、`Context::Set/Get`、`Object::Set/Get/CallMethod` 这类“用空 `MaybeLocal` / `Nothing` 报告失败”的 API，桥接层会在边界先尝试把 pending exception 转交给当前可接收的 `TryCatch`。
7. 若当前不存在可接收的 `TryCatch`，并且 `python_execution_depth == 0`（说明控制权已经回到宿主 C++ 边界），桥接层会清理 `Isolate` 上的 pending exception，避免旧异常污染后续调用。
8. 若当前不存在可接收的 `TryCatch`，但 `python_execution_depth > 0`，桥接层不会清理 pending exception；这是为了让异常继续回到解释器，由更外层 Python `try/except` 决定是否处理。
9. 这意味着：没有安装 `TryCatch` 时，你依然可以通过空返回值知道“这次 API 调用失败了”；只是拿不到异常对象本身。
10. `Isolate::ThrowException()` 是例外：它不依赖空 `MaybeLocal` / `Nothing` 报告失败，因此不会走“无人接收时自动清理边界异常”的结算路径。它只会向当前传播域注入 pending exception，并在条件满足时尝试立即转交给 Embedder `TryCatch`。

## 4.1 Isolate 契约（严格模式）

1. 所有带 `Isolate*` 入参的 API 要求“当前线程已绑定同一个 Isolate”。
2. 违反契约（如 `Current != explicit isolate`、`Current == null`）会直接终止进程。
3. 调用带 `Isolate*` 的 Embedder API 前，必须先建立 `Isolate::Scope`。
4. `HandleScope` 是 `Local<T>` 创建窗口的前置条件，但不负责 Isolate 绑定。
5. 跨线程共享同一 `Isolate` 时，必须先建立 `Locker` 再进入 `Isolate::Scope`。
6. `Locker` 支持同线程重入，但禁止跨线程解锁。

## 5. 一个实战 Demo 在哪里

1. 文件：`samples/game-engine-demo.cc`
2. 场景：宿主提供 `GetPlayerHealth/SetPlayerStatus`，Python 脚本实现怪物 AI 的 `on_update`。
3. 重点：演示 C++ -> Python -> C++ 双向调用，以及错误捕获路径。

## 6. 当前已知限制（请务必阅读）

### 6.1 内存与 OOM

1. 当前 OOM 不是以可恢复异常对外暴露。
2. 在底层分配失败的极端情况下，进程可能直接终止（例如 `std::exit(1)` 路径）。
3. 结论：MVP 阶段不承诺 OOM 可恢复。

### 6.2 并发与线程

1. 同一个 `Isolate` 不保证并发执行安全。
2. 跨线程共享同一 `Isolate` 时，必须使用 `Locker` 串行化访问。
3. 若要并行吞吐，强烈建议使用多个 `Isolate` 并做好外部线程隔离。

### 6.3 前端编译开关

1. `Script::Compile/Run` 的完整脚本体验依赖 `SAAUSO_ENABLE_CPYTHON_COMPILER`。
2. 在未开启该能力时，`Compile` 失败是预期行为。

### 6.4 兼容性说明

1. 当前为 MVP 迭代期，不承诺稳定二进制 ABI。
2. 升级版本时请重新编译宿主工程。

## 7. 接入建议

1. 先用 `samples/hello-world.cc` 跑通 `Isolate::Scope + HandleScope` 生命周期。
2. 再跑 `samples/game-engine-demo.cc` 验证互调能力。
3. 一组隔离业务使用多个 `Context`，不要复用同一个 globals 承载不同租户状态。
4. 统一使用 `Context::Scope` 管理 Enter/Exit，避免手工错序退出。
5. 在业务代码里统一封装 `TryCatch` + `MaybeLocal` 判空模板，避免漏判。
6. 对宿主回调参数做显式校验，错误统一走 `ThrowRuntimeError` 或 `Exception` 工厂。
