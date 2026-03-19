# S.A.A.U.S.O Embedder API Phase 0-2 架构与代码审查报告 (CR)

## A. 审查总评

**总体等级：B+**
当前实现基本建立了一个边界清晰、V8 风格的嵌入式 API 骨架。API 隔离度好（成功屏蔽了内部的 `Tagged<T>` 等细节），构建层隔离严谨，`TryCatch` 与 `Isolate` 的栈式生命周期设计在思路上是正确的。
然而，在**内存生命周期管理、C++ 语言细节（如成员遮蔽）**以及**错误传播契约**上存在明显的隐患。目前架构处于“能跑但不健壮”的阶段，尤其面临严重的内存泄漏和 UAF (Use-After-Free) 风险。

**结论：建议在修复 P0/P1 问题后，再正式进入 Phase 3 的开发。**

---

## B. 架构亮点 (做得好的地方)

1. **头文件与实现隔离彻底**：`include/saauso-embedder.h` 极其干净，内部实现全部通过 `ApiAccess` 和不透明的 `void* impl_` 隐藏在 `src/api/api.cc` 中。这为未来修改内部 `ValueImpl` 布局提供了完全自由，不破坏 ABI。
2. **构建边界清晰**：`BUILD.gn` 中将 `embedder_api` 单独抽离，并约束了 `include_dirs`。确保了外部（如 `test/embedder/*`）绝对无法 `#include "src/..."`，在物理层面切断了泄漏途径。
3. **异常契约初步成型**：`TryCatch` 采用栈式管理（`TryCatchTop` 与 `previous_`），且 `Script::Run` 在返回空 `MaybeLocal` 前会主动调用 `CapturePendingException` 将内部异常转换到外部。
4. **编译时环境分发健壮**：正确利用了 `#if SAAUSO_ENABLE_CPYTHON_COMPILER`，并分别对有/无前端编译器的执行行为编写了测试用例。

---

## C. 问题与风险清单

### P0 风险（阻碍进入下一阶段，必须立即修复）

#### 1. API 层 Value 对象严重内存泄漏
* **问题描述**：所有 `saauso::Local<T>` 指向的 C++ 对象（如通过 `String::New` 或 `Script::Run` 创建的 `Value`）都被 push 到 `Isolate::values_` 向量中。这些对象**只有在 `Isolate::Dispose()` 时才会被统一释放**。
* **影响**：如果宿主在一个常驻线程中循环执行 Python 脚本或频繁创建字符串，C++ 内存会无限增长。现有的 `saauso::HandleScope` 仅释放了内部 VM 句柄（`internal::Handle`），但未能管理 API 层的 `Value` 对象。
* **位置**：`src/api/api.cc` 中的 `WrapObject`、`WrapHostString` 等。

#### 2. 二次释放与 UAF (Use-After-Free) 风险
* **问题描述**：`saauso::Value` 拥有公有的虚析构函数。如果 Embedder 误调用 `delete *local_val;`，该对象会被释放。但在 `Isolate::Dispose()` 中，`values_` 仍保留着这个指针，会再次调用 `DestroyValue`。
* **影响**：极易触发双删导致整个宿主进程 Crash。
* **位置**：`include/saauso-embedder.h` 第 30 行，以及 `api.cc` 第 197 行的 `Dispose` 循环。

#### 3. Context 成员遮蔽 (Shadowing) 导致野指针
* **问题描述**：`Context` 继承自 `Value`，但在 `Context` 中又重复定义了私有成员 `Isolate* isolate_{nullptr};` 和 `void* impl_{nullptr};`。这覆盖了父类 `Value` 中的同名成员。
* **影响**：当对 `Context` 对象调用 `Value` 的虚方法或类型转换时，`GetValueImpl` 拿到的是父类的 `impl_`（为 nullptr），会导致段错误。
* **位置**：`include/saauso-embedder.h` 第 140-141 行。

---

### P1 风险（架构一致性与可用性）

#### 4. `Script::Compile` 异常契约不一致
* **问题描述**：在没有 CPython 前端时（`#else` 分支），`Script::Compile` 依然返回了“成功”的 `Script` 对象（只封装了源码），错误被延迟到了 `Script::Run` 阶段才抛出。
* **影响**：破坏了“尽早失败”的原则。Embedder 会误以为编译成功。
* **位置**：`test/embedder/embedder-script-unittest.cc` 第 81 行。

#### 5. TryCatch 未显式重置时的状态说明
* **问题描述**：如果 C++ 端抛出了异常被 `TryCatch` 捕获，但宿主没有调用 `TryCatch::Reset()` 或未处理就离开了作用域。
* **影响**：虽然内部 `ExceptionState` 已被 `CapturePendingException` 清理，但外部缺乏文档说明：离开 TryCatch 作用域等同于异常被“吞掉”。

---

### P2 风险（后续优化项）

#### 6. ValueImpl 内存开销较大
* **问题描述**：每个 `Value` 都配了一个 `ValueImpl` 并在堆上分配（`new ValueImpl()`）。对于整数这种轻量类型，每次都要两次堆分配（`new Integer` + `new ValueImpl`），性能较差。
* **建议**：在 Phase 3/4 优化时，考虑将 `ValueImpl` 的结构通过 `union` 压缩，或在 64 位系统下利用 NaN-boxing 或 Tagged pointer 直接在 `impl_` 指针位存储小整数，消除二次堆分配。

---

## D. 测试补充建议 (Gate 条件)

在进入 Phase 3 前，必须补齐以下测试用例：

1. **`HandleScope_Prevents_Memory_Leak`**：在 `for` 循环中创建 10000 个 `String::New`，在每个循环内放置 `HandleScope`。循环结束后，验证 `Isolate` 的 `values_` 注册表大小没有增长。
2. **`TryCatch_Nested_Capture`**：创建两个嵌套的 `TryCatch`，内层抛错后 `HasCaught()` 为 true。外层应不受影响。测试作用域链恢复的正确性。
3. **`Context_Is_Valid_Value`**：将 `Context` cast 为 `Value`，调用 `IsString()` 应该安全返回 false 而不崩溃。

---

## E. 下一步行动建议

1. **冻结新功能开发**：暂停 Phase 3 (Callback & Interop) 的开发。
2. **执行专项整改**：针对上述 P0 和 P1 问题，向负责开发的 AI Agent 发送整改 Prompt，要求其在 1-2 个 Commit 内修复内存管理和 C++ 继承问题。
3. **验收测试**：运行新增的 3 个核心测试用例，确保全部通过。
4. **推进 Phase 3**：整改通过后，正式进入 Phase 3。
