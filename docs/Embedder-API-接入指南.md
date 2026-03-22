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

3. `Script`：待执行的脚本文本
   - `Script::Compile` 相当于“把字符串脚本编译成可执行对象”。
   - `Run` 才是真正执行。
   - 任何一步失败都可能返回空 `MaybeLocal`，并由 `TryCatch` 捕获错误。

4. `Local<T>`：一个“受作用域管理”的安全引用
   - 它不是裸指针，而是 API 层的句柄包装。
   - `Local<T>` 的生命周期受 `HandleScope` 管理，离开作用域后不应继续使用。
   - 这样做是为了防止内存泄漏和悬挂引用。

5. `MaybeLocal<T>`：可能有值，也可能失败
   - 成功时可 `ToLocal` 取出结果。
   - 失败时是空值（`IsEmpty()==true`），必须配合 `TryCatch` 看具体异常。
   - 这是 Embedder API 里最重要的防御式调用约定。

6. `HandleScope`：一段“临时对象自动回收区间”
   - 在这段区间创建的包装对象会被统一管理。
   - 区间结束自动回收，避免长期累积导致内存膨胀。
   - `EscapableHandleScope` 则允许你把一个返回值“逃逸”到外层继续使用。

7. `TryCatch`：错误观察器
   - 只要你要调用可能失败的 API（编译、运行、方法调用），就建议先创建 `TryCatch`。
   - 调用后检查 `HasCaught()`，这是判定“脚本侧是否抛错”的标准方式。

一句话串起来：  
`Isolate` 提供运行沙箱，`Context` 提供全局环境，`Script` 提供可执行逻辑，`Local/MaybeLocal` 提供安全结果传递，`HandleScope/TryCatch` 提供内存与异常保障。

## 2. 5 分钟上手（带代码片段）

下面这段代码就是一个最小可运行模板：初始化引擎、创建上下文、注入一个全局值、执行脚本、读取结果、处理异常、最后释放资源。

```cpp
#include "saauso-embedder.h"
#include "saauso.h"

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) return 1;

  {
    saauso::HandleScope scope(isolate);
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    if (context.IsEmpty()) return 1;

    context->Set(saauso::String::New(isolate, "hp"),
                 saauso::Local<saauso::Value>::Cast(saauso::Integer::New(isolate, 100)));

    saauso::TryCatch try_catch(isolate);
    saauso::MaybeLocal<saauso::Script> maybe_script =
        saauso::Script::Compile(isolate, saauso::String::New(isolate, "hp = hp - 10\n"));
    if (maybe_script.IsEmpty() || try_catch.HasCaught()) return 1;

    saauso::MaybeLocal<saauso::Value> run_result = maybe_script.ToLocalChecked()->Run(context);
    if (run_result.IsEmpty() || try_catch.HasCaught()) return 1;

    saauso::MaybeLocal<saauso::Value> hp_value = context->Get(saauso::String::New(isolate, "hp"));
    if (!hp_value.IsEmpty()) {
      int64_t hp = 0;
      saauso::Local<saauso::Value> hp_local;
      if (hp_value.ToLocal(&hp_local) && hp_local->ToInteger(&hp)) {
      }
    }
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return 0;
}
```

你可以按这个顺序记忆：

1. `Initialize`：启动运行时
2. `Isolate::New`：创建脚本沙箱
3. `HandleScope + Context::New`：进入受控作用域并拿到执行环境
4. `Set/Get`：和脚本共享数据
5. `Compile + Run`：执行脚本逻辑
6. `TryCatch + MaybeLocal`：捕获失败，避免崩溃式调用
7. `Dispose`：释放资源

当你需要更复杂交互时，把第 4 步从 `Context::Set/Get` 升级为 `Context::Global()` + `Object::CallMethod`，即可实现“对象式 API 调用”。

## 2.1 5 分钟上手（回调版）

这一版演示最关键能力：用 `Function::New` 把 C++ 回调注入到脚本，然后由脚本反向调用宿主。

```cpp
#include "saauso-embedder.h"
#include "saauso.h"

namespace {
int64_t g_last_damage = 0;

void HostApplyDamage(saauso::FunctionCallbackInfo& info) {
  int64_t damage = 0;
  if (!info.GetIntegerArg(0, &damage)) {
    info.ThrowRuntimeError("ApplyDamage expects integer");
    return;
  }
  g_last_damage = damage;
  info.SetReturnValue(
      saauso::Local<saauso::Value>::Cast(saauso::Integer::New(info.GetIsolate(), damage)));
}
}  // namespace

int main() {
  saauso::Saauso::Initialize();
  saauso::Isolate* isolate = saauso::Isolate::New();
  if (isolate == nullptr) return 1;

  {
    saauso::HandleScope scope(isolate);
    saauso::Local<saauso::Context> context = saauso::Context::New(isolate);
    if (context.IsEmpty()) return 1;

    saauso::Local<saauso::Function> apply_damage =
        saauso::Function::New(isolate, &HostApplyDamage, "ApplyDamage");
    context->Set(saauso::String::New(isolate, "ApplyDamage"),
                 saauso::Local<saauso::Value>::Cast(apply_damage));

    saauso::TryCatch try_catch(isolate);
    saauso::MaybeLocal<saauso::Script> maybe_script = saauso::Script::Compile(
        isolate, saauso::String::New(isolate, "ApplyDamage(25)\n"));
    if (maybe_script.IsEmpty() || try_catch.HasCaught()) return 1;
    if (maybe_script.ToLocalChecked()->Run(context).IsEmpty() ||
        try_catch.HasCaught()) {
      return 1;
    }
  }

  isolate->Dispose();
  saauso::Saauso::Dispose();
  return g_last_damage == 25 ? 0 : 1;
}
```

回调版你只要记住 4 步：

1. 定义 C++ 回调函数（从 `FunctionCallbackInfo` 取参数、设返回值）
2. 用 `Function::New` 生成函数对象
3. `Context::Set` 注入到脚本全局
4. 脚本里直接调用该函数，必要时用 `TryCatch` 观察异常

## 3. 常用 API 速览

### 3.1 基础生命周期

1. `Isolate::New/Dispose`
2. `HandleScope`
3. `Context::New/Enter/Exit`
4. `Context::Global`（返回全局字典对应的 `Object`）

### 3.2 Context 适用场景与语义

1. `Context::New` 语义
   - 每次调用都创建一个新的执行环境（独立 globals）。
   - 适用于“同一 Isolate 下多业务域隔离”（如房间、插件、会话、脚本租户）。

2. `Enter/Exit` 语义
   - 每个 Isolate 内维护 Context 栈，`Enter` 入栈，`Exit` 只允许退出栈顶 Context（LIFO）。
   - `ContextScope` 是 RAII 版本：构造时 `Enter`，析构时 `Exit`。
   - 建议优先使用 `ContextScope`，避免手动配对错误。

3. 隔离效果
   - 变量隔离：`context_a` 的全局变量不会出现在 `context_b`。
   - 回调绑定隔离：只注入到 `context_a` 的宿主函数不会自动出现在 `context_b`。
   - 异常隔离：某个 Context 中的脚本失败不会污染另一个 Context 的后续执行路径。

### 3.3 值类型

1. `String` / `Integer` / `Float` / `Boolean`
2. `Object`：`Set/Get/CallMethod`
3. `List`：`Length/Push/Set/Get`
4. `Tuple`：`New/Length/Get`

### 3.4 回调与异常

1. `Function::New`（注册宿主回调）
2. `Function::Call`
3. `FunctionCallbackInfo`（读参数、写返回值、`ThrowRuntimeError`）
4. `TryCatch`（统一捕获 API 失败）
5. `Exception::TypeError/RuntimeError`（MVP 工厂接口）
6. `Isolate::ThrowException`（宿主主动注入异常）

## 4. 错误处理规范（必须遵守）

1. 所有可失败 API 都可能返回空 `MaybeLocal<T>`。
2. 只要你关心失败原因，就在调用前创建 `TryCatch`。
3. `MaybeLocal` 失败 + `TryCatch.HasCaught()==true` 才是完整错误闭环。
4. 不要忽略空句柄；必须先判空再 `ToLocalChecked`。

## 4.1 Isolate 契约（严格模式）

1. 所有带 `Isolate*` 入参的 API 要求“当前线程已绑定同一个 Isolate”。
2. 违反契约（如 `Current != explicit isolate`、`Current == null`）会直接终止进程。
3. `HandleScope` 是进入 API 调用窗口的前置条件，`Local<T>` 创建前必须先建立 `HandleScope`。
4. 不允许跨线程直接复用同一 `Isolate` 执行 Embedder API。

## 5. 一个实战 Demo 在哪里

1. 文件：`samples/game_engine_demo.cc`
2. 场景：宿主提供 `GetPlayerHealth/SetPlayerStatus`，Python 脚本实现怪物 AI 的 `on_update`。
3. 重点：演示 C++ -> Python -> C++ 双向调用，以及错误捕获路径。

## 6. 当前已知限制（请务必阅读）

### 6.1 内存与 OOM

1. 当前 OOM 不是以可恢复异常对外暴露。
2. 在底层分配失败的极端情况下，进程可能直接终止（例如 `std::exit(1)` 路径）。
3. 结论：MVP 阶段不承诺 OOM 可恢复。

### 6.2 并发与线程

1. `Isolate` 目前按单线程访问模型使用。
2. 不保证同一个 `Isolate` 可被多个线程并发调用 API。
3. 若要并行，请使用多个 `Isolate` 并做好外部线程隔离。

### 6.3 前端编译开关

1. `Script::Compile/Run` 的完整脚本体验依赖 `SAAUSO_ENABLE_CPYTHON_COMPILER`。
2. 在未开启该能力时，`Compile` 失败是预期行为。

### 6.4 兼容性说明

1. 当前为 MVP 迭代期，不承诺稳定二进制 ABI。
2. 升级版本时请重新编译宿主工程。

## 7. 接入建议

1. 先用 `samples/hello-world.cc` 跑通生命周期。
2. 再跑 `samples/game_engine_demo.cc` 验证互调能力。
3. 一组隔离业务使用多个 `Context`，不要复用同一个 globals 承载不同租户状态。
4. 统一使用 `ContextScope` 管理 Enter/Exit，避免手工错序退出。
5. 在业务代码里统一封装 `TryCatch` + `MaybeLocal` 判空模板，避免漏判。
6. 对宿主回调参数做显式校验，错误统一走 `ThrowRuntimeError` 或 `Exception` 工厂。
