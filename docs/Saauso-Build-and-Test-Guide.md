# S.A.A.U.S.O 构建与测试指南

本文档收口 S.A.A.U.S.O 的构建、测试与单元测试架构说明，适用于人类开发者与 AI 助手的日常开发回归。

## 1. 构建

根 [BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/BUILD.gn) 当前主要提供：

- `saauso_core`：核心实现 `source_set`（虚拟机主体）。
- `saauso_cpython312_compiler`：嵌入式 CPython 3.12 编译器前端的可选依赖门面（开关关闭时不引入任何 CPython 依赖）。
- `embedder_api`：公共 Embedder API 目标，暴露 `include/` 头并链接 `src/api/` 桥接实现。
- `vm`：示例入口（见 [main.cc](file:///e:/MyProject/S.A.A.U.S.O/src/main.cc)）。
- `embedder_hello_world` / `embedder_game_engine_demo`：Embedder API 示例程序。
- `all_ut`：统一测试入口（聚合 `//test/unittests:ut` 与 `//test/embedder:embedder_ut`）。

### 1.1 构建开关

- `saauso_enable_cpython_compiler`（默认 `true`）：控制是否启用“内嵌 CPython 3.12 编译器前端”。
  - 该开关会注入 C++ 宏 `SAAUSO_ENABLE_CPYTHON_COMPILER=0/1`，用于隔离依赖 `Python.h` 的代码路径。
  - 当关闭时：
    - `Compiler::CompileSource`、`Runtime_ExecutePythonSourceCode` 与 `exec(str)` 会 fail-fast（提示需要开启该开关）。
    - import 路径不再搜索 `.py`（仅搜索 `.pyc`）；若未命中则按常规导入错误抛出 `ModuleNotFoundError`。
    - `CompilePyc` 与导入 `.pyc` 仍可用。
    - 单元测试会自动剔除依赖源码编译的解释器端到端用例与编译器前端用例，仅保留纯后端相关用例用于回归。
  - 开关默认值与宏注入位置：
    - 默认值：`build/config/BUILDCONFIG.gn`
    - 宏注入：`build/BUILD.gn`

### 1.2 构建工具与环境

项目本地包含了 `depot_tools`（Windows/Linux amd64），可直接调用其中的 `gn`/`ninja`。

注意：仓库脚本 [build.sh](file:///e:/MyProject/S.A.A.U.S.O/build.sh) 默认直接调用 `gn`/`ninja`（Windows 下为 `gn.exe`/`ninja.exe`），因此需要确保它们在 PATH 中可用。

- 推荐做法：在 Bash 环境中把 `depot_tools/` 加入 PATH（或自行安装并配置 gn/ninja）。
- 若不想配置 PATH：在 Windows PowerShell 下可以直接使用 `.\depot_tools\gn.exe` / `.\depot_tools\ninja.exe` 命令手动构建。

Windows 上默认使用 Clang/LLD 工具链（见 `build/` 与 `build/toolchain/`），ASan 链接依赖 `llvm_lib_path`（默认 `D:\LLVM\lib\clang\21\lib\windows`）。如果你本地 LLVM 安装路径不同，请通过 `gn args` 或 `--args="llvm_lib_path=... is_asan=true"` 覆盖。

为增强跨平台兼容性检查，本项目在 Windows 与 Linux 的编译配置中均启用了较严格的警告集合（例如 `-Wextra/-Wshadow/-Wunreachable-code`）并将警告视为错误（`-Werror`）。这要求新增代码在两端都保持“零警告构建”（Clean Build）。

### 1.3 推荐构建方式（脚本）

推荐优先使用仓库自带脚本（需要 Bash 环境，如 Git Bash/MSYS2/WSL）：

```bash
./build.sh release
./build.sh debug
./build.sh asan
./build.sh ut
./build.sh ut_backend
```

`build.sh` 在 `gn gen` 后会自动执行一次 `gn check`，用于检查 `BUILD.gn` 中的 `deps/public_deps/visibility`、头文件可达性与 include 合法性；若该步骤失败，应先修复依赖边界问题，再继续编译。

### 1.4 Windows PowerShell 手动构建

在 Windows PowerShell 下可直接手动执行（常用目标名为 `vm`/`all_ut`；其中 `ut.exe` 与 `embedder_ut.exe` 是分别运行的测试二进制）：

```powershell
# 生成构建文件
.\depot_tools\gn.exe gen out/debug --args="is_debug=true"

# 检查依赖、可见性与 include 合法性
.\depot_tools\gn.exe check out/debug

# 构建
.\depot_tools\ninja.exe -C out/debug vm

# 导出 compile_commands.json (供 clangd/IDE 使用)
.\depot_tools\gn.exe gen out/debug --args="is_debug=true" --export-compile-commands
```

无论是人类开发者还是 AI 助手，只要完成了代码、头文件、`BUILD.gn`、`deps/public_deps/visibility` 或新增源文件的修改，在交付前都应至少执行一次 `gn check`；若使用 `build.sh`，该检查会自动完成。

## 2. 测试

测试分为三层：

- `test/unittests/`：核心 VM 的 GTest 单元测试。
- `test/embedder/`：Embedder API 的 GTest 测试。
- `test/python312/`：不依赖 GTest 的 Python 脚本回归样例。

根 `BUILD.gn` 只负责聚合测试入口 `all_ut`；真正可执行的测试二进制仍然分别位于 `test/unittests:ut` 与 `test/embedder:embedder_ut`。

```powershell
# 构建统一测试入口
.\depot_tools\gn.exe gen out/ut --args="is_asan=true"
.\depot_tools\ninja.exe -C out/ut all_ut

# 分别运行两套 GTest
.\out\ut\ut.exe
.\out\ut\embedder_ut.exe
```

## 3. 单元测试架构

### 3.1. 核心 VM 的 GTest 单元测试

- **公共测试代码（当前布局）**
  - `test/unittests/test-helpers.{h,cc}`：提供带生命周期的测试夹具基类（负责 `Saauso::Initialize/Dispose`、`Isolate` 创建与 Enter/Exit 等）与解释器输出捕获夹具。
  - `test/unittests/test-utils.{h,cc}`：提供无状态的小工具与断言谓词（例如 `IsPyStringEqual`、`AppendExpected`、pyc 字节构造器等）。
- **测试目录分层（按模块分类）**
  - `test/unittests/interpreter/`：解释器相关用例，已按专题拆分为较细的测试矩阵；新增用例前务必先阅读文档 [Interpreter 单测目录说明文档](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/interpreter/README.md) 并优先归入既有专题文件。
  - `test/unittests/objects/`：对象系统与容器/属性相关用例（`py-*-unittest.cc`、`attribute-unittest.cc`）。
  - `test/unittests/heap/`：GC/堆相关用例（如 `gc-unittest.cc`）。
  - `test/unittests/handles/`：句柄与并发相关用例（如 `handle-*-unittest.cc`、`global-handle-unittest.cc`）。
  - `test/unittests/code/`：pyc 解析/编译前端相关用例（如 `pyc-file-parser-unittest.cc`）。
  - `test/unittests/utils/`：纯工具/算法相关用例（如 `string-search-unittest.cc`、`random-number-generator-unittest.cc`）。
- **统一夹具基类**
  - `VmTestBase`：适用于绝大多数“需要完整虚拟机环境”的单测。
  - `IsolateOnlyTestBase`：仅创建/销毁 `Isolate`（不 Enter），用于线程隔离类测试。
- **解释器端到端测试**
  - 统一使用 `BasicInterpreterTest` 夹具（print 注入 + 输出捕获）。
  - 用例按主题拆分到 `interpreter-*-unittest.cc`，避免单文件职责膨胀。
- **前端开关对测试的影响**
  - 当 `saauso_enable_cpython_compiler=false` 时，[test/unittests/BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/BUILD.gn) 会自动剔除依赖源码编译的解释器端到端用例与编译器前端用例，仅保留纯后端相关用例用于回归（对应脚本模式 `./build.sh ut_backend`）。
- **LSan 抑制**
  - `test/unittests/lsan-suppressions.cc`
- **新增测试文件的接入点**
  - 新增 `test/unittests/**/**.cc` 后，需要同步将其加入 [test/unittests/BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/BUILD.gn) 的 `ut` 目标 `sources` 列表。

### 3.2. Embedder API 的 GTest 测试
- **LSan 抑制**
  - `test/embedder/lsan-suppressions.cc`
- **新增测试文件的接入点**
  - 新增 `test/embedder/*.cc` 后，需要同步将其加入 [test/embedder/BUILD.gn](file:///e:/MyProject/S.A.A.U.S.O/test/embedder/BUILD.gn) 的 `embedder_ut` 目标 `sources` 列表。
