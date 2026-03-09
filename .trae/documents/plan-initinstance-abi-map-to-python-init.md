# InitInstance ABI 对齐 Python `__init__` 的整改计划

## 1. 结论先行（对你观察的评估）

你的观察成立：

* 目前 `InitInstance` 的 ABI 返回 `Maybe<void>`，只表达“成功/失败”，不表达 Python 语义层面 `__init__` 的返回对象。

* Python 语义要求 `__init__` 返回 `None`（显式返回非 `None` 会报错），因此从“可映射性/一致性”角度，当前 C++ ABI 与 Python 语义存在间隙。

* 若希望将 C++ `InitInstance` 直接映射到 Python 世界 `__init__`，ABI 改为 `MaybeHandle<PyObject>` 并在协议层强制“成功时返回 `None`”更合理。

## 2. 整改目标

1. 将 `InitInstance` ABI 从 `Maybe<void>` 调整为 `MaybeHandle<PyObject>`。
2. 在运行时门面层统一约束：`InitInstance` 成功返回值必须是 `None`，否则抛 `TypeError("__init__() should return None, not 'X'")`。
3. 完成“C++ `InitInstance` 可映射到 Python `__init__`”的协议落地（统一把 init 视为返回 `PyObject` 的可调用约定）。
4. 保持现有成功/异常行为与错误传播机制（pending exception）稳定。

## 3. 实施方案

### 3.1 ABI 与接口层改造

涉及文件：

* `src/objects/klass.h`

* `src/objects/klass.cc`

* `src/objects/klass-default-vtable.cc`

改动要点：

* `VirtualTable` 中 `init_instance` 函数指针类型由 `Maybe<void>` 改为 `MaybeHandle<PyObject>`。

* `Klass::InitInstance(...)` 返回值同步改为 `MaybeHandle<PyObject>`。

* 默认实现 `Virtual_Default_InitInstance(...)` 返回 `MaybeHandle<PyObject>`，其行为为：

  * 有 `__init__`：执行调用并返回调用结果；

  * 无 `__init__`：返回 `None`。

### 3.2 Runtime 门面语义收敛

涉及文件：

* `src/runtime/runtime-reflection.cc`

改动要点：

* `Runtime_NewObject(...)` 调用 `InitInstance` 后增加返回值检查：

  * 若返回非 `None`：抛出 `TypeError("__init__() should return None, not '%s'")`，并返回空 `MaybeHandle`；

  * 若返回 `None`：正常返回已构造实例。

### 3.3 各内建类型 `InitInstance` 适配

涉及文件（至少）：

* `src/objects/py-list-klass.h/.cc`

* `src/objects/py-dict-klass.h/.cc`

* `src/objects/py-smi-klass.h/.cc`

* `src/objects/py-float-klass.h/.cc`

* `src/objects/py-type-object-klass.h/.cc`

* （按检索结果补齐其他已定义 `Virtual_InitInstance` 的类型）

改动要点：

* 所有 `Virtual_InitInstance` 签名改为返回 `MaybeHandle<PyObject>`。

* 语义统一：

  * 成功：返回 `None`；

  * 失败：返回空 `MaybeHandle` 并保留 pending exception。

* `tuple/str` 等当前走默认 `InitInstance` 的类型无需额外逻辑，但需要验证不回归。

### 3.4 文档与规范同步

涉及文件：

* `AGENTS.md` 中构造协议章节

改动要点：

* 明确 `init_instance` 语义为“Python `__init__` 的 C++ 映射入口，返回值协议为 `None`”。

* 补充“非 `None` 返回触发 `TypeError`”的规则。

## 4. 单元测试计划（含你给的案例）

### 4.1 新增/更新解释器端到端测试

优先位置：

* `test/unittests/interpreter/interpreter-custom-class-unittest.cc`

* 视覆盖情况可增补到 `interpreter-builtins-seq-unittest.cc` / `interpreter-dict-unittest.cc`

至少覆盖以下用例：

1. **你提供的回归样例（list 子类）**

   * 脚本：

   ```python
   class L(list):
     def __init__(self, iterable, x):
       list.__init__(self, iterable)
       self.x = x

   l = L([1,2,3], 100)
   print(l[0] + l[1] + l[2] + l.x)
   ```

   * 期望输出：`106`

2. **`__init__`** **显式返回** **`None`** **正常**

   * 自定义类/内建子类场景各至少 1 例。

3. **`__init__`** **返回非** **`None`** **抛错**

   * `return 1` / `return "x"` 等，校验 `TypeError` 文案与触发时机。

4. **无** **`__init__`** **场景**

   * 验证默认路径返回 `None`（对用户可见行为是构造成功）。

5. **dict/list 子类参数转发**

   * 确认 ABI 改造后参数语义与既有行为不变。

### 4.2 构建与回归验证

执行策略：

* 全量构建：`ninja -C out/Default`

* 定向测试：聚焦 `BuiltinsConstructors`、`DictConstructor`、custom class 与新增 case

* 必要时补跑全量 `ut.exe` 以确认无跨模块副作用。

## 5. 风险与控制

1. **风险：ABI 改签引发大面积编译错误**

   * 控制：按“核心接口 -> 各类型实现 -> 运行时门面 -> 测试”顺序改，逐层编译收敛。
2. **风险：错误文案与 CPython 不一致**

   * 控制：统一在 `Runtime_NewObject` 做文案集中校验，避免分散实现。
3. **风险：内建类型** **`InitInstance`** **返回值遗漏**

   * 控制：全仓检索 `Virtual_InitInstance(`，确保全部改签并返回 `None/empty`。

## 6. 交付标准

* `InitInstance` ABI 已升级为 `MaybeHandle<PyObject>`。

* `Runtime_NewObject` 已强制 `__init__` 返回 `None` 语义。

* 你的 `L(list)` 样例回归通过。

* 新增“`__init__` 返回非 `None` 报错”测试并通过。

* 全量构建通过，核心构造相关单测通过。

