# S.A.A.U.S.O 项目结构与阅读路线

本文档收口 S.A.A.U.S.O 的仓库目录结构速览与建议阅读路线，用于帮助贡献者快速建立模块边界与源码索引心智模型。

## 1. 仓库结构速览

- `include/`：Embedder API 的公共 ABI 层；对外暴露 `Isolate/Context/Script/Value/Object/Function/TryCatch/MaybeLocal` 等接口，以及少量必要的内部基础定义（如 `saauso-internal.h`）。
- `src/api/`：Embedder API 桥接层，负责 `Local/MaybeLocal` 与 VM 内部对象句柄之间的转换、异常捕获与 API 级防御性语义。
- `src/init/`：全局运行时生命周期（`Saauso::Initialize/Dispose`），负责嵌入式 CPython312 前端的 Setup/TearDown。
- `src/execution/`：运行时容器（`Isolate`）与隔离性/多线程访问控制（`thread_local` Current + `Isolate::Scope/Locker`），并编排各 `Klass` 的初始化顺序；同时包含执行门面 `Execution`（AllStatic）与异常类型清单 `exception-types.h`。当前脚本执行统一经由 `Execution::Call*` / `Execution::RunScriptAsMain` 收口。
- `src/builtins/`：内建函数与“内建类型方法集合”的实现层；内建函数统一使用 `Builtin_*` 函数签名；内建类型方法集合通常由各 `PyXxxKlass::Initialize()` 安装到 type dict（例如 `str/list/dict/tuple/type` 及其 iterator/view 的 methods）；此外包含 Accessor 子系统（`accessor-proxy`/`accessors`），用于 `__class__`/`__dict__` 等保留属性访问代理与约束处理。
- `src/code/`：编译/`.pyc` 解析前端（`Compiler`、`cpython312-pyc-file-parser`、`cpython312-pyc-compiler` 等）。
- `build/`：构建配置、工具链与编译控制宏（如 `BUILDFLAG`、`IS_WIN` 等）；同时包含 GN 工具链配置与 Embedder include 边界检查脚本。
- `src/common/`：跨模块共享的轻量公共定义（当前以 `globals.h` 等为主）。
- `src/interpreter/`：字节码解释器内部实现（bytecode dispatcher、`FrameObject` 栈帧、参数归一化与调用算法）；不再作为顶层脚本执行的外部入口。
- `src/modules/`：模块系统（import）：`ModuleManager` 持有 `sys.modules/sys.path` 并作为解释器入口；`ModuleImporter` 编排 dotted-name/fromlist/相对导入；`ModuleNameResolver` 解析相对导入基准；`ModuleFinder` 只做文件系统定位；`ModuleLoader` 负责创建模块对象并执行模块体；`ModuleUtils` 提供模块名合法性、package 判定与 `__path__` 提取等通用小工具；`BuiltinModuleRegistry` 管理内建模块注册表。
- `src/objects/`：对象系统（`PyObject`、`Klass`、各内建对象与其 `*-klass`）。
- `src/handles/`：句柄系统（`Handle`/`HandleScope`/`HandleScopeImplementer`）、长期句柄 `Global<T>` 与 `Tagged<T>`。
- `src/heap/`：堆与空间（`NewSpace`/`OldSpace`/`MetaSpace`）以及新生代 GC（Scavenge）。
- `src/runtime/`：运行时 helper（通用 runtime helper、字符串表 StringTable 等）。内建函数实现已下沉到 `src/builtins/`。
- `src/utils/`：通用工具（对齐/内存/小型容器、BinaryFileReader 等）。该目录下的代码严禁依赖和调用虚拟机的任何上层能力。
- `samples/`：Embedder API 示例程序（当前含 `hello-world`、`game-engine-demo`），用于验证公共 API 的可用性与典型接入方式。
- `test/embedder/`：Embedder API 的 GTest 测试，覆盖句柄线程约束、Isolate/Locker、脚本执行与宿主互调等外部嵌入场景。
- `test/python312/`：端到端 Python 脚本样例（不依赖 GTest）。
- `test/unittests/`：基于 GTest 的单元测试。
- `BUILD.gn`：根目标定义（核心 `saauso_core`、可选 `saauso_cpython312_compiler`、公共 `embedder_api`、CLI `vm`、Embedder 示例、统一测试入口 `all_ut`）。
- `build/`：编译配置与工具链（Clang/LLD，支持 `is_debug`/`is_asan`）；其中 `check_embedder_includes.py` 用于约束 Embedder 示例/测试只能依赖 `include/` 公共头，不得直接 `#include "src/..."`。
- `testing/gtest/`：项目内置的 GTest GN 封装目标（供 `ut` 与 `embedder_ut` 等单测目标复用）。
- `third_party/`：第三方代码（例如 `googletest` 上游源码、`rapidhash`、`fast_float`、以及可选的 `cpython312` 头文件与静态库）。

## 2. 建议阅读路线（快速上手）

- 外部嵌入视角：从 [saauso.h](file:///e:/MyProject/S.A.A.U.S.O/include/saauso.h) 与 Embedder 文档体系（[Saauso-Embedder-API-User-Guide.md](file:///e:/MyProject/S.A.A.U.S.O/docs/Saauso-Embedder-API-User-Guide.md)、[Embedder-API-Architecture-Design.md](file:///e:/MyProject/S.A.A.U.S.O/docs/architecture/Embedder-API-Architecture-Design.md)）建立公共 API 的分层与生命周期心智模型，再进入 [src/api/](file:///e:/MyProject/S.A.A.U.S.O/src/api) 看桥接实现。
- 生命周期与 CLI 入口：从 [main.cc](file:///e:/MyProject/S.A.A.U.S.O/src/main.cc) 看开发调试入口；CLI 主脚本启动已通过 `Execution::RunScriptAsMain(...)` 收敛到 execution 门面。开启前端时默认跑内嵌源码 demo，关闭 `saauso_enable_cpython_compiler` 时走 `.pyc` 执行路径。
- 全局运行时初始化：读 [saauso.cc](file:///e:/MyProject/S.A.A.U.S.O/src/init/saauso.cc)（嵌入式 CPython312 前端生命周期）。
- 编译链路：读 [compiler.cc](file:///e:/MyProject/S.A.A.U.S.O/src/code/compiler.cc)（source -> pyc bytes -> PyCodeObject）。
- 运行时与初始化顺序：读 [isolate.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/isolate.cc)（`Init/InitMetaArea/TearDown`）。
- 错误与异常门面：读 [exception-state.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-state.h)、[exception-utils.h](file:///e:/MyProject/S.A.A.U.S.O/src/execution/exception-utils.h) 与 [runtime-exceptions.h](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-exceptions.h)（pending exception + Runtime_Throw* + 便捷宏）。
- 字节码执行主循环：读 [interpreter-dispatcher.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter-dispatcher.cc)（computed-goto handlers）。
- 调用与参数绑定：先读 [execution.cc](file:///e:/MyProject/S.A.A.U.S.O/src/execution/execution.cc) 理解对外执行门面，再读 [interpreter.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/interpreter.cc) 与 [frame-object-builder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/frame-object-builder.cc) 看底层调用与栈帧构建实现。
- 模块系统（import）：读 [module-manager.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-manager.cc)（sys.modules/sys.path 与入口）、[module-importer.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-importer.cc)（dotted-name/fromlist 编排与父子绑定）、[module-name-resolver.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-name-resolver.cc)（相对导入解析）、[module-finder.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-finder.cc)（文件系统定位规则）、[module-loader.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/module-loader.cc)（创建模块对象并执行模块体）、[builtin-sys-module.cc](file:///e:/MyProject/S.A.A.U.S.O/src/modules/builtin-sys-module.cc)（sys 模块暴露）、[py-module.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-module.h)（模块对象）。
- builtins 注册与实现：读 [builtin-bootstrapper.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/builtin-bootstrapper.h)（builtins 字典装配）、[builtins-definitions.h](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-definitions.h) 与 `src/builtins/builtins-*.cc`（内建函数与类型方法集合），以及各 `PyXxxKlass::Initialize()` 中的 methods 安装入口。
- 对象模型与属性查找：读 [py-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.cc) 与 [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)。
- 异常表（exception table）：读 [exception-table.h](file:///e:/MyProject/S.A.A.U.S.O/src/interpreter/exception-table.h) 与 [py-code-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-code-object.h)（`co_exceptiontable` 的保存与解释器侧查找）。
- 堆与新生代 GC：读 [heap.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/heap.cc) / [spaces.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/spaces.cc) / [scavenge-visitor.cc](file:///e:/MyProject/S.A.A.U.S.O/src/heap/scavenge-visitor.cc)。
