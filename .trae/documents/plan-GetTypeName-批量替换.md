# PyObject::GetTypeName 批量替换计划

## Summary

* 目标：将虚拟机中“先 `GetKlass(...)` 再 `name()`”的高频类型名读取，尽量统一收敛到 `PyObject::GetTypeName(...)`，优先覆盖当前已存在且签名完全匹配的 `Handle<PyObject>` 场景。

* 范围：

  * 仅做调用点收敛与局部变量重命名/简化
  * 不改 `PyObject::GetTypeName` 的接口语义
  * 不顺带调整无关错误文案。

* 成功标准：仓库内主要 `Handle<PyObject>` 类型名读取热点完成替换，语义与报错文本保持不变，相关单测与构建回归通过。

## Current State Analysis

* 已存在统一入口：[py-object.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.h#L48-L49) 与 [py-object.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-object.cc#L103-L105) 提供 `PyObject::GetTypeName(Handle<PyObject>, Isolate*)`。

* 当前仓库中仍大量直接使用 `PyObject::GetKlass(x)->name()` 或等价写法。探索结果显示：

  * 精确链式模式约 48 处，涉及 `src/objects/`、`src/runtime/`、`src/builtins/`、`src/modules/`。

  * 另有约 4 处为已缓存 `*_klass` 后再取 `name()`。

* 高密度热点文件包括：

  * `src/objects/py-object-klass.cc`

  * `src/objects/py-smi-klass.cc`

  * `src/objects/py-string-klass.cc`

  * `src/objects/py-list-klass.cc`

  * `src/runtime/runtime-py-type-object.cc`

  * `src/objects/klass-vtable-trampolines.cc`

  * `src/modules/builtin-random-module.cc`

* 现有 `GetTypeName` 调用点已分布在 `src/builtins/accessors.cc`、`src/builtins/builtins-io.cc`、`src/builtins/builtins-exec.cc`，说明该 API 已被仓库接受，适合继续推广。

* 需特别区分两类场景：

  * 可直接替换：参数本身就是 `Handle<PyObject>`，且只需要类型名。

  * 保守处理：前文仍需复用 `klass` 做其他判断，或对象是 `Tagged<PyObject>`；这类点位只在不扩 API 的前提下做最小必要调整，避免引入额外行为变化。

## Proposed Changes

### 1. 批量收敛直接类型名读取

* 在以下文件中，将 `PyObject::GetKlass(obj)->name()` 或 `->name()->buffer()` 直接替换为 `PyObject::GetTypeName(obj, isolate)` 及其 `->buffer()` 形式：

  * `src/objects/py-object-klass.cc`

  * `src/objects/py-smi-klass.cc`

  * `src/objects/py-string-klass.cc`

  * `src/objects/py-float-klass.cc`

  * `src/objects/klass-vtable-trampolines.cc`

  * `src/runtime/runtime-reflection.cc`

  * `src/runtime/runtime-exceptions.cc`

  * `src/runtime/runtime-py-function.cc`

  * `src/runtime/runtime-py-string.cc`

  * `src/runtime/runtime-py-smi.cc`

  * `src/runtime/runtime-py-type-object.cc`

  * `src/modules/builtin-math-module.cc`

  * `src/modules/builtin-random-module.cc`

  * `src/modules/builtin-time-module.cc`

  * `src/builtins/builtins-py-list-methods.cc`

  * `src/builtins/builtins-py-object-methods.cc`

  * `src/builtins/builtins-py-dict-methods.cc`

  * `src/builtins/builtins-py-string-methods.cc`

* 处理方式保持机械化：

  * 链式报错场景直接改调用入口。

  * 已缓存 `type_name` 的场景改为 `auto type_name = PyObject::GetTypeName(..., isolate);`

  * 不修改错误格式串与参数顺序，确保行为等价。

### 2. 保留必要的 klass 语义，避免过度替换

* 对以下文件中的 `instance_klass` / `other_klass` 场景，先检查该 `klass` 是否还承担其他职责：

  * `src/objects/py-list-klass.cc`

  * `src/objects/py-dict-klass.cc`

  * `src/objects/py-base-exception-klass.cc`

  * `src/objects/py-string-klass.cc`

* 若 `klass` 仅用于取 `name()`，则改为 `GetTypeName(...)` 并移除无意义中间变量。

* 若 `klass` 同时参与 `vtable`、布局或类型判断，则保留 `klass` 变量，只把 `klass->name()` 这一步替换为直接对对象调用 `GetTypeName(...)`；不为“风格统一”破坏现有局部可读性。

### 3. 明确本次不动的边界

* 不修改 `PyObject::GetTypeName` 的实现与声明。

* 不新增 `Tagged<PyObject>` 版本的 `GetTypeName`。

* 不处理纯 `Tagged<PyObject>` 类型名读取热点，例如 `src/runtime/runtime-conversions.cc`，因为这会额外引入 API 设计决策，超出本次“推广既有新 API”的范围。

* 不借机改动 unrelated cleanup（例如错误文案措辞、局部 helper 抽取、注释补齐之外的结构性重构）。

## Assumptions & Decisions

* 假设本次“类似于 `PyObject::GetKlass(object)->name()` 的操作”优先指向现有 `Handle<PyObject>` 场景；这是与当前 API 签名最一致、收益最高且风险最低的收敛方式。

* 决策：采用“语义等价、最小扰动”的机械替换策略，不扩接口，不改变 `klass` 的其他用途。

* 决策：若某个调用点缺少 `isolate` 形参但当前上下文可稳定获取同一作用域内的 `isolate` / `isolate_`，则使用该现有变量；若必须新增额外上下文获取逻辑，优先评估是否仍属于最小改动范围。

* 假设现有单测已覆盖大部分错误文案与类型检查路径，因此主要通过编译 + 相关单测回归验证替换安全性。

## Verification Steps

* 全量检索 `src/` 中 `PyObject::GetKlass(...)->name()` 与 `*_klass->name()`，确认：

  * 目标 `Handle<PyObject>` 场景已按计划收敛。

  * 剩余未替换点仅为计划中明确保留的 `Tagged<PyObject>` 或确有必要保留 `klass` 直取名称的特殊点。

* 构建并运行与受影响面最相关的测试集合：

  * `//test/unittests:ut`

  * 如需缩短反馈回路，可优先关注 `interpreter-str-unittest.cc`、`interpreter-list-exceptions-unittest.cc`、`interpreter-random-module-unittest.cc`、`interpreter-math-module-unittest.cc`、`py-object-unittest.cc`、`py-string-unittest.cc`、`py-list-unittest.cc` 所在集合。

* 若回归中出现编译失败，优先检查：

  * 调用点是否缺失 `isolate` 变量。

  * 局部变量类型是否从 `Tagged<Klass>` / `Handle<PyString>` 混用导致推导不一致。

  * `klass` 变量删除后是否误伤其他语义判断。

