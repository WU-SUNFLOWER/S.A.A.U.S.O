## 目标

在一个仓库内同时支持两种可发布形态：

1. **演示/开发形态**：内嵌 CPython 3.12 编译器前端，支持把源码（`.py`/`str`）即时编译为 `.pyc` 并执行。
2. **纯后端形态**：不包含任何 CPython 前端编译器依赖，仅提供“加载/解析/执行 `.pyc`”能力，面向 embedder 打包或网络下发字节码的场景。

为此引入 GN 编译开关 `saauso_enable_cpython_compiler`，并映射到 C++ 预处理宏 `SAAUSO_ENABLE_CPYTHON_COMPILER`，用于隔离依赖 CPython C-API 的代码与构建依赖。

## 非目标（本轮不做）

- 不引入新的异常传播体系；仍沿用当前工程的 `stderr + exit(1)` fail-fast 策略。
- 不强行让所有 GTest 用例在“纯后端形态”下也能跑通；本轮优先保证 **核心库可构建** 且 **不再链接 CPython**。

## 设计原则

1. **默认不破坏**：开关默认开启（保持现有 `vm/ut` 行为与开发体验不变）。
2. **依赖隔离彻底**：关闭开关时，任何 `Python.h`/`python312.lib` 的 include/link 都不应发生。
3. **行为明确可预期**：关闭开关后，所有“需要源码编译”的能力应明确 fail-fast（并提示如何启用开关）。
4. **embedder 友好**：关闭开关后，`CompilePyc`/`.pyc import` 等后端能力仍可用。

## 改动方案（按落点）

### 1) 新增 GN arg，并把它变成全局宏

- 文件：
  - `build/config/BUILDCONFIG.gn`
  - `build/BUILD.gn`
- 改动：
  - 在 `declare_args()` 中新增 `saauso_enable_cpython_compiler = true`。
  - 在 `config("compiler_defaults")` 中注入全局 `defines`：
    - `SAAUSO_ENABLE_CPYTHON_COMPILER=1`（开启）
    - `SAAUSO_ENABLE_CPYTHON_COMPILER=0`（关闭）

### 2) 隔离 CPython 前端 target（构建层依赖隔离）

- 文件：
  - 根 `BUILD.gn`
  - `test/unittests/BUILD.gn`
- 改动：
  - `source_set("saauso_cpython312_compiler")`：在 `saauso_enable_cpython_compiler=false` 时：
    - 不编译 `src/code/cpython312-pyc-compiler.*`
    - 不依赖 `//third_party/cpython312`
    - 允许其它 target 继续写 `deps += [ ":saauso_cpython312_compiler" ]`（但该 target 退化为空实现 target）
  - `executable("vm")`：
    - 若开关关闭：改为只依赖 `:saauso_core`，并在源码侧做行为降级（见第 4 节）。
  - 单测 `//test/unittests:ut`：
    - 仍允许在开关开启时保持现状依赖；
    - 开关关闭时，移除对 `//:saauso_cpython312_compiler` 的显式依赖，并在 sources 层面剔除“必须编译源码”的测试文件（见第 6 节）。

### 3) 隔离全局生命周期钩子（Saauso::Initialize/Dispose）

- 文件：
  - `src/init/saauso.cc`
- 改动：
  - `Saauso::Initialize/Dispose` 仍负责维护 `IsInitialized()` 的全局状态（Isolate 创建前置条件不变）。
  - 当 `SAAUSO_ENABLE_CPYTHON_COMPILER=0` 时：
    - 不 `#include "src/code/cpython312-pyc-compiler.h"`
    - `Initialize/Dispose` 不再调用 `EmbeddedPython312Compiler::Setup/TearDown`（变成纯后端 no-op）

### 4) 隔离“源码→字节码”的编译入口（Compiler / runtime / builtins / import）

- 文件：
  - `src/code/compiler.{h,cc}`
  - `src/runtime/runtime-exec.cc`
  - `src/builtins/builtins-exec.cc`
  - `src/modules/module-finder.cc`（或 `module-loader.cc`，按既有分层选择）
- 改动：
  - `Compiler::CompileSource(...)`：
    - 开关开启：行为不变
    - 开关关闭：`stderr` 打印清晰错误（提示需启用 `saauso_enable_cpython_compiler=true`），并 `exit(1)`
  - `Runtime_ExecutePythonSourceCode(...)`：
    - 开关关闭：同样 fail-fast（避免 import `.py` 或 `exec(str)` 走到隐式编译）
  - `builtins.exec`：
    - 当参数为 `str` 且开关关闭：fail-fast
    - 当参数为 `code`（或等价 code object）时：保持可执行（纯后端仍可用）
  - import 查找策略（embedder 友好）：
    - 开关开启：保持 “`.py` 优先、`.pyc` 兜底”
    - 开关关闭：跳过 `.py` 探测或调整优先级为 “`.pyc` 优先”（避免目录里同时存在 `.py` 时，纯后端形态被迫走源码路径）

### 5) 提供纯后端演示入口（不依赖源码编译）

- 文件：
  - `src/main.cc`
- 改动：
  - 开关开启：保持现有 demo（内嵌源码字符串 → `CompileSource` → 执行）
  - 开关关闭：提供一个最小可用路径，例如：
    - 从命令行参数读取 `.pyc` 文件路径（若未传参则提示用法并退出）
    - 走 `Compiler::CompilePyc(isolate, filename)` 执行字节码

### 6) 单测降级策略（仅保证“能编译”，不保证全量覆盖）

- 文件：
  - `test/unittests/BUILD.gn`（必要时配合少量 `#if`）
- 改动：
  - 在 `saauso_enable_cpython_compiler=false` 时剔除依赖 `RunScript/CompileSource/EmbeddedPython312Compiler` 的测试源文件（尤其是解释器端到端用例与 compiler 用例）。
  - 保留不依赖源码编译的测试（例如 utils/handles/heap/objects 中纯 C++ 侧用例），确保纯后端形态仍能进行基础回归。

### 7) 文档同步

- 文件：
  - `AGENTS.md`
- 改动：
  - 在“构建/发布形态”相关章节补充 `saauso_enable_cpython_compiler` 的背景（演示程序/纯虚拟机后端两种需求同时存在）、用途、使用方法（在构建时如何开启或关闭）、默认值、以及关闭后的行为差异（`.py`/`exec(str)` 不可用、`.pyc` 可用）。

## 验收与验证

1. **默认配置（开关开启）**：`vm`/`ut` 的构建与行为保持不变。
2. **纯后端配置（开关关闭）**：
   - `:saauso_core` 可编译链接；
   - 不再链接 `//third_party/cpython312`；
   - `vm` 能运行 `.pyc`（或至少给出明确的使用说明与失败信息）；
   - `.py` 导入与 `exec(str)` 会给出明确的 fail-fast 提示。
