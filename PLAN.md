## 目标

在不增加过多复杂度的前提下，让 S.A.A.U.S.O 的模块系统支持“仅分发 `.pyc` 字节码”的 embedder 场景，同时保持与 CPython 3.12 的整体直觉一致（源码优先、字节码可作为无源码模块被导入）。

## 设计原则（结合 CPython 3.12 与 S.A.A.U.S.O 场景）

1. **简单可预测**：导入器只在“与模块同目录/同层级”检查 `.pyc`，不默认扫描 `__pycache__` 的 tagged 缓存路径。
2. **开发友好**：当 `.py` 与 `.pyc` 同时存在时，**优先加载 `.py`**（与 CPython 常见直觉一致，也方便调试）。
3. **满足 embedder**：当找不到 `.py` 时，允许直接加载 `.pyc`（sourceless module/package）。
4. **保持分层**：`src/modules` 负责定位与装配模块对象；“执行 code / 注入 builtins / 驱动解释器”收敛在 `src/runtime` 的统一入口中，减少重复逻辑与未来扩展成本。
5. **MVP fail-fast**：沿用当前工程的错误处理策略（stderr + exit(1)），不引入复杂异常传播。

## 导入查找与加载规则（MVP）

对每个导入段（dotted-name 的每一段），在某个 base path 下按以下顺序尝试：

1. **Package**（目录形式）：
   - `<base>/<rel>/__init__.py`
   - `<base>/<rel>/__init__.pyc`
2. **Module**（文件形式）：
   - `<base>/<rel>.py`
   - `<base>/<rel>.pyc`

其中：
- `<rel>` 是相对模块路径（例如 `pkg/sub`）。
- 一旦命中，返回 `origin`（实际文件路径）、`is_package`、`package_dir`（package 时为目录路径，否则为空）、以及 `kind`（`.py` 或 `.pyc`）。
- **不**默认扫描 `__pycache__/<name>.cpython-312.pyc` 这类缓存路径；未来若需要“`.py` 存在时优先用缓存加速”，再单独加一套“cache 验证/选择”逻辑。

## 代码改动点（按落点）

### 1) 扩展模块定位结果：能表达 `.py` / `.pyc`

- 文件：
  - `src/modules/module-finder.h`
  - `src/modules/module-finder.cc`
- 改动：
  - 在 `ModuleLocation` 中加入 `kind` 字段（例如 `enum class ModuleFileKind { kSourcePy, kBytecodePyc };`）。
  - `FindModuleLocation(...)` 增加 `.pyc` 探测，并按“`.py` 优先、`.pyc` 兜底”返回。

### 2) 扩展加载路径：按 kind 分流执行

- 文件：
  - `src/modules/module-loader.cc`
- 改动：
  - `LoadModulePartImpl(...)` 在拿到 `ModuleLocation` 后：
    - `kSourcePy`：保持现状（读源码 → `Runtime_ExecutePythonSourceCode`）。
    - `kBytecodePyc`：跳过读源码，直接调用新的 runtime 执行入口（见下一节），把 `origin` 作为 filename 传入。

### 3) 新增统一执行入口：执行 `.pyc` 文件

- 文件：
  - `src/runtime/runtime.h`
  - `src/runtime/runtime-exec.cc`
- 改动：
  - 新增 `Runtime_ExecutePythonPycFile(std::string_view filename, Handle<PyDict> locals, Handle<PyDict> globals)`。
  - 实现逻辑与 `Runtime_ExecutePythonSourceCode` 保持一致：
    - 若 `globals` 缺 `__builtins__` 则注入。
    - `Compiler::CompilePyc(Isolate::Current(), filename)` 得到 `PyFunction`。
    - 绑定 globals 并 `interpreter->CallPython(...)` 在指定字典中执行模块代码。

## 单测策略（验证对 embedder 场景“必须可用”）

在现有 `test/unittests/interpreter/interpreter-import-unittest.cc` 中增加一个用例，验证：

1. 运行时动态生成一个 `.pyc` 文件（不落地 `.py`）：
   - 使用 `EmbeddedPython312Compiler::CompileToPycBytes(source, filename)` 生成字节码。
   - 将 bytes 写入临时目录下的 `<modname>.pyc`。
2. 在解释器脚本中：
   - `sys.path.append(<临时目录>)`
   - `import <modname>` 并 `print(<modname>.x)` 等，确认模块执行生效。

该用例覆盖：模块系统 `.py` 缺失时的 `.pyc` fallback、模块缓存、以及执行入口能正确注入 builtins 并运行。

## 交付验收

1. `import` 在 `.py` 存在时行为不变。
2. 当目标模块仅存在 `.pyc`（同目录）时，`import` 成功并正确执行模块体。
3. `ut` 目标可编译并通过新增用例。
4. 将模块导入规则同步进`AGENTS.md`，便于后续的AI助手和程序员理解。
