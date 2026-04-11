## Summary

* 目标：在 `Isolate` 中引入 `ExceptionRoots`，把 VM 内部异常类型的权威来源从 `builtins` 字典切换为稳定 roots 槽位；`builtins` 仅保留对 Python 层的发布职责。

* 本次范围：按“核心闭环”实施，不做全仓库异常查找路径的大规模扩散替换。

* 约束：严格遵守 [Saauso-Coding-Style-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Coding-Style-Guide.md) 的代码规范要求；优先保持现有 `Isolate` / X-macro / GC root 遍历风格。

## Current State Analysis

* 当前 `Runtime_NewExceptionInstance()` 通过 `exception_type_name -> isolate->builtins() -> PyDict::Get()` 查异常类型，再实例化异常对象；`Runtime_ConsumePendingStopIterationIfSet()` 也依赖 `builtins["StopIteration"]`。[runtime-exceptions.cc:L106-L142](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc#L106-L142) [runtime-exceptions.cc:L217-L247](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.cc#L217-L247)

* 当前异常类型的创建与发布都集中在 `BuiltinBootstrapper`：`BaseException` 作为 builtin type 直接安装，`Exception` 与其他普通异常通过 `Runtime_CreatePythonClass()` 动态创建后写入 `builtins`。[builtin-bootstrapper.cc:L81-L110](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc#L81-L110) [builtin-bootstrapper.cc:L163-L231](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc#L163-L231)

* 这意味着当前 `BuiltinBootstrapper` 同时承担“异常元数据构造者”和“Python 可见性发布者”两种职责；若长期目标是让 `ExceptionRoots` 成为 VM 内部单点真相，则把“创建异常类型”下沉到 `ExceptionRoots` 内部会比只在 bootstrap 阶段顺手登记 roots 更彻底。

* `Isolate` 目前只直接持有 `builtins_`，没有异常类型 root 容器；`Isolate::Iterate()` 当前也只暴露 `builtins_` 给 GC。[isolate.h:L88-L90](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.h#L88-L90) [isolate.h:L141-L143](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.h#L141-L143) [isolate.cc:L218-L221](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc#L218-L221)

* 异常类型枚举定义集中在 `exception-types.h`，适合作为 ExceptionRoots 槽位与 bootstrap 注册的单一真相源。[exception-types.h:L28-L52](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-types.h#L28-L52)

* 现有与 bootstrap 直接相关的回归测试集中在 `interpreter-builtins-bootstrap-unittest.cc`，但尚未验证“ExceptionRoots 已写入且与 builtins 中发布对象同一性一致”。[interpreter-builtins-bootstrap-unittest.cc:L103-L164](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc#L103-L164)

## Assumptions & Decisions

* 决策 1：采用独立的 `ExceptionRoots` 小型封装，而不是在业务代码里直接暴露 `FixedArray + enum 下标`。

  * 原因：兼顾你希望的“按 `ExceptionType` 槽位访问”与仓库当前偏好的“具名/封装访问器”，避免后续在 runtime 代码中扩散裸下标访问。

* 决策 2：`ExceptionRoots` 的底层存储使用 `FixedArray`，但对外暴露 `Get(ExceptionType)` / `Set(ExceptionType, ...)` 以及必要的 typed helper。

  * 原因：底层贴合你的 roots slot 设计；接口层避免魔法下标，降低易错性。

* 决策 3：`BaseException` 也纳入 `ExceptionRoots`，保持 `ExceptionType` 全量一一对应。

  * 原因：避免“`BaseException` 例外处理”形成双重真相；后续 runtime 若需要统一按 `ExceptionType` 取 type object，可直接复用同一接口。

* 决策 4：本次只替换核心 runtime 消费路径，不主动扫描并替换其他未来可能出现的 `builtins` 异常类型查找点。

  * 具体包含：异常实例创建、`StopIteration` 消费、bootstrap 初始化、GC root 暴露、核心单测。

* 决策 5：保留 `GetExceptionStringHandle()` 作为“异常类型名字符串”辅助，仅用于需要名字的场景；不再作为异常类型对象的主查找路径。

* 决策 6：新增/修改头文件声明、内部 helper 与非显而易见逻辑时，按编码规范补充必要中文注释。

* 决策 7：采纳“异常类型创建逻辑下沉到 `ExceptionRoots`”的分层方案。

  * 原因：这样 `ExceptionRoots` 不只是被动缓存，而是直接成为异常类型对象的构造与持有中心；`BuiltinBootstrapper` 则收敛为单纯的 Python 可见性发布层，职责边界比原方案更清晰。

## Proposed Changes

### 1. 新增 `ExceptionRoots` 封装

* 文件：

  * `src/execution/exception-roots.h`

  * `src/execution/exception-roots.cc`

  * `BUILD.gn`

* 设计：

  * 新增 `ExceptionRoots` 类，内部持有 `Tagged<FixedArray>`，封装异常类型 root 槽表，并负责完成异常类型对象的创建、登记与发布前准备。

  * 提供以下最小接口：

    * `ExceptionRoots(Isolate* isolate)`

    * `Maybe<void> Setup();`

    * `Handle<PyTypeObject> Get(ExceptionType type) const;`

    * `void Set(ExceptionType type, Handle<PyTypeObject> value);`

    * `Maybe<void> InitializeBuiltinExceptionTypes();`

    * `void Iterate(ObjectVisitor* v);`

  * 在 `exception-types.h` 中为 `ExceptionType` 增加稳定容量终点（例如 `kCount`），供 `FixedArray` 分配与边界断言使用。

* 实现要点：

  * 槽位索引直接来自 `static_cast<int>(ExceptionType::kXxx)`。

  * `Get/Set` 内部做集中断言，避免调用方分散手写容量与 cast。

  * `InitializeBuiltinExceptionTypes()` 内部负责：

    1. 把 `BaseException` 的 type object 登记到 `kBaseException` 槽位；
    2. 创建 `Exception` 类型并登记到 `kException` 槽位；
    3. 基于 `NORMAL_EXCEPTION_TYPE` 批量创建其余异常类型并写入对应槽位。

  * 头文件声明与关键 helper 位置补中文注释，说明“为何使用 roots 作为 VM 内部权威来源”。

### 2. 扩展 `Isolate` 持有与 GC 暴露

* 文件：

  * `src/execution/isolate.h`

  * `src/execution/isolate.cc`

* 改动：

  * 新增 `ExceptionRoots* exception_roots_` 或更贴合现有风格的专用字段，并暴露：

    * `ExceptionRoots* exception_roots();`

  * 在 `Isolate::Init()` 中，先创建`ExceptionRoots`单例，完成 `ExceptionRoots::Setup()`（内部会进一步调用 `ExceptionRoots::InitializeBuiltinExceptionTypes()`），最后再执行 builtins bootstrap 发布逻辑。

  * 在 `Isolate::Iterate()` 中显式暴露 `exception_roots_`，让 GC 直接看到这一组长期异常类型对象。

  * 在 `TearDown()` 中按既有风格清空 / 释放对应状态。

* 选择理由：

  * 让 `Isolate` 成为异常类型 roots 的单点真相；

  * 避免 runtime 继续依赖 `builtins` 的动态名字查找。

### 3. 重构 `BuiltinBootstrapper` 为“只负责发布 ExceptionRoots 中的对象”

* 文件：

  * `src/interpreter/builtin-bootstrapper.h`

  * `src/interpreter/builtin-bootstrapper.cc`

* 改动：

  * 把现有“创建异常类型并写入 builtins”的职责拆走，仅保留“把 `ExceptionRoots` 里的同一批 type object 发布到 `builtins`”。

  * 删除或重写 `InstallBuiltinBasicExceptionTypes()` / `RegisterSimpleTypeToBuiltins(...)` 这类承担异常类型创建职责的 helper，改为更贴切的发布型 helper，例如：

    * `Maybe<void> PublishBuiltinExceptionTypes();`

    * `Maybe<void> PublishExceptionType(ExceptionType type, Handle<PyString> type_name);`

  * `InstallBuiltinTypes()` 仍负责安装基础 builtin types，但对 `BaseException` 的 Python 可见性发布改为统一走“从 `ExceptionRoots` 取对象再写入 builtins”的路径，避免双入口。

* 预期结果：

  * bootstrap 结束后，`ExceptionRoots[type]` 与 `builtins[name]` 指向同一个 `PyTypeObject`；

  * `BuiltinBootstrapper` 从“异常构造 + 发布”双职责收敛为单纯的 Python 层发布面。

### 4. 重写 runtime 异常主路径为 roots 读取

* 文件：

  * `src/runtime/runtime-exceptions.h`

  * `src/runtime/runtime-exceptions.cc`

* 改动：

  * 把 `Runtime_NewExceptionInstance(...)` 的入口从“按名字找异常类型”调整为“按 `ExceptionType` 直接取 type object”。

  * 推荐签名改为：

    * `MaybeHandle<PyObject> Runtime_NewExceptionInstance(Isolate* isolate, ExceptionType type, Handle<PyString> message_or_null);`

  * `Runtime_ThrowError()` 直接将 `ExceptionType` 传入 `Runtime_NewExceptionInstance()`，不再走 `GetExceptionStringHandle()` + builtins 查找。

  * `Runtime_ConsumePendingStopIterationIfSet()` 直接使用 `isolate->exception_roots()->Get(ExceptionType::kStopIteration)` 做 `isinstance` 判定。

  * `Runtime_FormatPendingExceptionForStderr()` 继续按实例真实类型名格式化，不做语义变更。

* 边界处理：

  * roots 读取失败/空槽位应视为 bootstrap 违例，优先以 `assert` 守住内部不变量，而不是退回 builtins 查找。

### 5. 增补测试，覆盖 roots 与 builtins 的一致性

* 文件：

  * `test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc`

  * 如有必要：`test/unittests/interpreter/interpreter-exceptions-unittest.cc`

* 计划新增验证：

  * `BaseException/Exception/TypeError/.../StopIteration` 等异常类型已写入 `ExceptionRoots`。

  * `ExceptionRoots` 中取出的对象与 `builtins` 中发布的对象是同一引用。

  * `Runtime_ThrowError()` 仍可生成正确异常实例，现有异常消息格式化测试不回退。

  * `for` 循环消费 `StopIteration` 的行为保持不变。

* 策略：

  * 优先在 bootstrap 单测中增加“roots 与 builtins 同一性”断言；

  * 若 runtime 改签名导致现有测试不再充分，再在异常行为测试中补一条针对 roots 主路径的回归。

## File-by-File Execution Plan

* `src/execution/exception-types.h`

  * 新增容量终点枚举值，用于 roots 槽表大小。

* `src/execution/exception-roots.h/.cc`

  * 新建异常 roots 封装，内部管理 `FixedArray` 槽位与访问接口。

* `src/execution/isolate.h/.cc`

  * 新增 `exception_roots_` 持有、访问器、初始化时机与 GC root 暴露。

* `src/interpreter/builtin-bootstrapper.h/.cc`

  * 删除异常类型创建职责，只保留“把 `ExceptionRoots` 中的异常类型对象发布到 builtins”。

* `src/runtime/runtime-exceptions.h/.cc`

  * 把异常实例创建与 `StopIteration` 判定切换为读 `ExceptionRoots`。

* `BUILD.gn`

  * 注册新增的 `exception-roots.h/.cc`。

* `test/unittests/interpreter/interpreter-builtins-bootstrap-unittest.cc`

  * 增加 roots 存在性与 roots/builtins 同一性验证。

* `test/unittests/interpreter/interpreter-exceptions-unittest.cc`（如需要）

  * 保底覆盖 `StopIteration` 路径无回归。

## Verification

* 构建验证：

  * 运行单测目标，确保新增源文件已被 GN 正确纳入构建。

* 行为验证：

  * `interpreter-builtins-bootstrap-unittest` 通过，证明异常类型既存在于 builtins，也存在于 `ExceptionRoots`，且对象同一。

  * 解释器异常相关单测通过，证明 `Runtime_ThrowError()` 与 `StopIteration` 消费路径未回归。

* 结构验证：

  * 代码审查确认 runtime 中不再存在“`ExceptionType -> name -> builtins lookup`”主路径。

  * `Isolate::Iterate()` 已显式遍历 `exception_roots_`，满足长期 GC root 清晰性目标。

