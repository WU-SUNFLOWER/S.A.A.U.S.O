## 解释器 Bootstrap 治理方案（参考 V8 思路）

### 1. 背景与问题

当前 `Interpreter::Interpreter` 承担了过多“启动装配（bootstrap）”职责：创建 `builtins` 字典、注入基础类型名/单例、注册 native builtins、以及创建一批内建异常类型。随着虚拟机功能演进，这段逻辑会持续膨胀，违反单一职责，并且让后续增量演进（新增 builtin、按开关裁剪、单测覆盖、启动性能优化）变得越来越困难。

现状代码位置：

* 解释器构造函数：[interpreter.cc:L43-L143](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc#L43-L143)

* builtins 的消费点（`LOAD_GLOBAL` 先查 globals，miss 查 builtins）：[interpreter-dispatcher.cc:L588-L619](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc#L588-L619)

### 2. V8 的 bootstrap 思路（对我们有启发的部分）

V8 的“bootstrap”并不是把所有初始化都塞进 `Isolate` 或执行器构造函数里，而是做了分层拆分，核心目标是：把“初始化阶段的装配复杂度”从“运行期热路径”与“核心执行器对象”中抽离出来，并且让功能可按模块扩展/裁剪。

对 SAAUSO 最有借鉴价值的要点（不追求一比一复刻）：

* **把环境搭建抽象成专门的 bootstrap 组件**：V8 有专门的 bootstrapper（以及 `Genesis` 概念）负责创建/配置执行上下文、安装内建对象等，而不是让执行循环对象承载所有装配逻辑（参考：V8 源码路径 `src/init/bootstrapper.cc`，以及 V8 官方博客对“bootstrapper 初始化一些对象、更多初始化交给自宿主 JS” 的描述）[web: V8 extras blog](https://v8.dev/blog/v8-extras)。

* **把“可扩展的内建”做成声明式/表驱动的安装过程**：V8 内建很多逻辑在自宿主 JS 中，并通过 snapshot/启动镜像加速；C++ 侧负责少量 glue 与关键对象的创建/链接。这种“表驱动 + 分层安装”使得新增 builtin 不需要改动核心构造函数的结构。

* **把初始化时序显式化**：先有最小根对象/容器，再逐步安装更高层对象；每一层只依赖之前层的产物，降低环依赖与“初始化顺序地狱”。

备注：SAAUSO 目前还不具备 V8 那种“snapshot + self-hosted language builtins”的基础设施，因此本方案只借鉴其“职责分离/表驱动/时序显式化”的架构治理思路。

### 3. 治理目标（这次改造要达到什么）

功能目标：

* 保持现有 Python 语义不变：`builtins` 中已有条目（类型名、`True/False/None`、`print/len/isinstance/build_class/sysgc/exec`、异常类型集合）仍可用。

* 让新增/删减 builtin 的改动收敛到“单点注册处”，避免不断扩大 `Interpreter::Interpreter`。

* 让 builtins 的初始化具备可单测性（至少能验证：名字存在、对象类型正确、关键依赖（如 `build_class`）可用）。

工程目标：

* `Interpreter::Interpreter` 保持“短小、只做成员初始化与调用 bootstrapper”的骨架。

* 把初始化逻辑拆成多个小函数/小模块，每个模块职责单一、可复用、可按编译开关裁剪。

* 把字符串 key 的获取策略集中化（逐步从 `PyString::NewInstance("...")` 过渡到 `StringTable` 的 `ST(...)`，减少重复分配与散落常量）。

### 4. 建议的目标架构（对标 V8 的治理方式，但贴合 SAAUSO 分层）

#### 4.1 新增一个“BuiltinBootstrapper”层（推荐落点：`src/interpreter/`）

新增组件（命名约定）：

* `BuiltinBootstrapper`：只负责创建并填充 `builtins` 字典，保持职责足够聚焦与单一。

* 后续若需要治理入口 `globals`、模块预注入等，再在不破坏现有抽象的前提下扩展为更高层的 `InterpreterBootstrapper`（或其它更贴切的命名），但本次改造先不引入该更宽泛抽象。

推荐 API（示例级，不是最终签名）：

* `Handle<PyDict> BuiltinBootstrapper::CreateBuiltins(Isolate* isolate);`

* `void BuiltinBootstrapper::InstallAll(Handle<PyDict> builtins);`

`Interpreter::Interpreter` 改为：

* `builtins_ = *BuiltinBootstrapper(isolate_).CreateBuiltins();`

#### 4.2 按“类别”拆分安装步骤（把时序显式化）

将当前构造函数中的逻辑拆分为 4 类 installer（每个 installer 一个小函数/小文件）：

1. 基础类型名（type objects）安装
2. oddballs（`True/False/None`）安装
3. native builtins（`print/len/...`）安装
4. MVP 异常类型安装（`BaseException/Exception/...`）

每一步只依赖：

* `Isolate`（访问 type\_object 与 oddballs 单例）

* `PyDict` 基础操作

* `Runtime_CreatePythonClass`（仅异常类型安装需要）

#### 4.3 用“表驱动”替换“散落的 Put”逻辑（降低未来膨胀速度）

对类型名、oddballs、native builtins 的安装使用表驱动：

* 类型名表：`{"int", PySmiKlass::GetInstance()->type_object()}` 形式

* oddballs 表：`{"None", isolate_->py_none_object()}` 形式

* builtin function 表：`{"len", &BUILTIN_FUNC_NAME(Len)}` 形式

优点：

* 新增 builtin 只改表，不改流程结构。

* 更容易按编译开关裁剪（例如 `exec` 依赖 `saauso_enable_cpython_compiler` 时可在表中条件编译）。

* 更容易写单测：表就是“期望集合”的来源。

#### 4.4 异常类型的进一步治理（可选增强，但建议纳入这次治理）

当前异常类型安装是构造函数里的闭包 + 两层基类初始化。
建议抽成：

* `Handle<PyTypeObject> CreateAndRegisterExceptionType(const char* name, Handle<PyTypeObject> base, Handle<PyDict> builtins);`

* 或者将“异常类型集合”的声明变成表（`TypeError -> Exception` 等）。

这样可以：

* 控制初始化顺序更清晰；

* 后续补齐更多异常类型时，不增加构造函数复杂度；

* 方便把“异常体系完善”作为独立演进方向继续扩展。

### 5. 迁移步骤（增量、可回滚）

#### Step 1：纯重构（不引入新文件也可先做）

* 在 `Interpreter` 内部先抽出私有 helper：

  * `RegisterBuiltinTypes(...)`

  * `RegisterOddballs(...)`

  * `RegisterBuiltinFunctions(...)`

  * `BootstrapBuiltinExceptions(...)`

* 验证行为不变（编译 + 现有 UT）。

#### Step 2：引入 bootstrapper 文件（把构造函数瘦身到“骨架”）

* 新增 `src/interpreter/interpreter-bootstrapper.{h,cc}`（或同等命名）。

* 把 Step 1 的 helper 迁移进去，`Interpreter::Interpreter` 只调用 bootstrapper 创建的 builtins。

* 更新 `BUILD.gn` 接入新文件。

#### Step 3：表驱动化 + 字符串 key 集中管理

* 在 bootstrapper 内部把硬编码 `Put` 改为表循环。

* 把高频 key（如 `object/int/str/.../True/False/None/print/len/...`）逐步迁移到 `StringTable`：新增对应 `ST(...)` 条目，统一从 `ST` 取 key。

* 确保不会引入 `src/utils` 方向的依赖（保持分层）。

#### Step 4：补齐/加固单测（防止回归）

新增或扩展解释器端到端单测/对象单测，至少覆盖：

* `builtins` 含 `object/int/str/...`，且值为 `PyTypeObject`

* `builtins["True"] is True`、`builtins["None"] is None`

* `print/len/isinstance/build_class/exec` 可被调用（可用最小脚本验证）

* 异常类型存在且 `isinstance(TypeError(), Exception)`（若当前异常对象语义尚不完整，则退化为“类型对象可用于 except 匹配”的最小验证）

注意：新增 UT 源文件需要同步加入 `test/unittests/BUILD.gn` 的 `sources`。

### 6. 风险点与应对

* **初始化顺序耦合**：异常类型创建依赖 `Runtime_CreatePythonClass` 与 `object_type`。通过“installer 分层 + 顺序显式化”规避。

* **字符串 key 的身份语义**：如果某些路径依赖字符串驻留（identity）或 hash 行为，迁移到 `StringTable` 需逐步进行，并用单测覆盖。

* **编译开关裁剪**：若未来 `exec` 与 `CompileSource` 的开关更强耦合，需要在表驱动处显式条件编译，避免运行期才 fail-fast。

### 7. 验收标准（完成后你能直观看到的效果）

* `Interpreter::Interpreter` 逻辑缩短为“创建 builtins + 调用 bootstrapper + 写入成员”。

* 新增 builtin 仅需：

  * 在表/注册点追加一行；

  * 必要时补齐单测；

  * 不需要修改解释器构造函数主体结构。

* 全量构建与 `ut` 通过，且 builtins 相关单测能覆盖主要条目。

