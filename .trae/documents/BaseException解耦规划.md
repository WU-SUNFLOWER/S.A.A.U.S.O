## BaseException 解耦规划（Plan）

### 1. 结论（先给判断标准）

从长期演进与 code health 的角度看，**“BaseException/Exception 及其子类的安装逻辑”不应该长期耦合在解释器层**；但这并不意味着要把它下沉到 `src/objects/`。更合理的方向是：

* **保留 objects 层仅承载对象表示与 vtable slot**（如 `PyObject/PyTypeObject` 及其布局、GC iterate、slot 分发骨架）。

* **把“内建异常类型的构建/注册”视为 runtime 语义的一部分**，放到 `src/runtime/` 或 `src/execution/` 的专用 bootstrap 组件中，再由 `BuiltinBootstrapper` 调用完成注入。

### 2. 现状评估：当前放在 builtin-bootstrapper.cc 正常吗？

现状文件：[builtin-bootstrapper.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.cc)

合理性（为什么短期可以接受）：

* 异常类型属于“启动装配（bootstrap）”的一部分，和 `builtins` 字典初始化同一阶段发生，放在一个 bootstrap 组件里在 MVP 阶段是合理折中。

* 当前实现通过 `Runtime_CreatePythonClass` 创建类型对象，属于“运行时语义”，并未触碰对象布局/GC 约束，因此不需要进入 objects 层。

风险（为什么长期不理想）：

* **层次边界模糊**：异常体系是全局语义能力，不应该由解释器目录下的组件成为唯一落点，否则未来任何异常语义演进都会被迫触碰解释器模块，增加环依赖风险。

* **文件体积膨胀**：异常类型数量会持续增长（`OSError` 系、`ImportError` 系、`StopIteration`/`GeneratorExit` 等），还会引入 `__cause__/__context__/__traceback__`、格式化、`raise` 语义等配套逻辑；继续塞在一个 bootstrapper 文件会越来越难维护与单测。

* **可复用性差**：若未来引入新的执行器（AOT/JIT）或“最小 runtime”裁剪，希望共享异常类型安装逻辑时，解释器目录会成为不必要的耦合点。

### 3. 是否需要下沉到 objects 层？

不建议，除非出现“异常对象需要特殊对象布局/额外 GC 字段/性能关键的专用表示”这类强理由。

原因：

* `src/objects/` 的职责是“对象表示 + slot 分发骨架”，不应承担“Python 语义层的类创建与内建名绑定”。

* 当前异常类型是通过 `Runtime_CreatePythonClass` 动态创建的 `PyTypeObject`，并非一个需要专用 C++ 对象布局的新类型；把它移入 objects 会把语义层代码（创建类、注入方法、错误文案）拉低到表示层，破坏依赖方向。

因此，**“解耦”更应该是从 interpreter/bootstrap 中抽离到 runtime/execution，而不是下沉到 objects。**

### 4. 推荐目标架构（最小扰动）

保持现有调用点与启动顺序不变，只做“职责搬迁”：

* 继续由 `BuiltinBootstrapper` 负责创建 builtins dict 并按步骤安装（types/oddballs/functions/...）。

* 将异常类型安装拆为独立模块，例如：

  * `src/runtime/runtime-exception-types.{h,cc}`（推荐）

  * 或 `src/execution/exception-bootstrapper.{h,cc}`（备选）

* 对外暴露一个清晰 API（示例级）：

  * `void Runtime_InstallBuiltinExceptionTypes(Handle<PyDict> builtins);`

  * 或 `MaybeHandle<PyObject> Runtime_InstallBuiltinExceptionTypes(Handle<PyDict> builtins);`（若未来安装过程可能抛 Python 异常）

核心原则：

* runtime/builtins 不应 include interpreter（保持依赖方向单向）。

* 安装过程只依赖 `Isolate`、`StringTable(ST)`、`Runtime_CreatePythonClass` 等“语义层能力”。

### 5. 迁移步骤（增量、可回滚）

1. **抽取异常类型安装模块**

   * 从 `BuiltinBootstrapper::InstallBuiltinExceptionTypes()` 与 `InstallBuiltinBasicExceptionTypes()` 中抽离代码到新文件。

   * 保留 `BuiltinBootstrapper` 中的调用骨架与安装顺序不变。

2. **收敛对 StringTable 的使用**

   * 优先让异常类型名与 dunder 名使用 `ST(...)`（避免重复分配与散落字面量）。

3. **补齐/加固单测**

   * 至少覆盖：`builtins` 中异常类型名存在；`BaseException`/`Exception` 的继承链正确；`str(e)`/`repr(e)` 的最小可用性不回归。

### 6. 验收标准

* `src/interpreter/builtin-bootstrapper.cc` 不再直接包含“异常类型列表/安装细节”，只保留对 runtime 安装函数的调用。

* 新模块不依赖 `src/interpreter/`，并能被其它启动路径复用。

* 全量构建与 `ut` 通过，且异常相关用例覆盖关键回归点。

