# print/str/repr 对齐 CPython312 的改造计划

## 1. 结论先行（关于你的记忆）

* 你的记忆**基本准确**：CPython 3.12 的对象协议中核心是 `tp_str` 与 `tp_repr`，没有独立的“print 虚方法”槽位。

* `print(...)` 在 CPython 中走“把对象按 `str()` 语义写出”的路径；`repr()` 是独立 builtin，且会在 `tp_str` 缺失时作为 `PyObject_Str` 的回退语义参与。

* 因此，S.A.A.U.S.O 现有 `Klass::vtable_.print_` 属于历史兼容层，应迁移为 `str/repr` 统一分发并移除旧入口。

## 2. 目标与边界

### 2.1 本次目标

1. 让内建对象与 trampoline 的 `str/repr` 行为收敛到 CPython 风格。
2. 重构 `builtins.print`，不再调用 `PyObject::Print`，改为基于 `str/repr` 转换输出。
3. 彻底删除对象系统中的 `print` 虚方法槽与调用链。
4. 增补/扩展单测，覆盖关键回归路径。

### 2.2 对齐边界（默认假设）

* 以“当前 VM 能力范围内的 CPython 对齐”为准：

  * 完整对齐 `sep/end` 语义与类型校验；

  * 移除自定义 `eol` 关键字；

  * `print` 对参数对象使用 `str` 语义；

  * `repr` 通过独立 builtin 与 slot 路径验证。

* 对 `file/flush`：先按现有运行时可承载能力实现最小兼容（`file=None` 主路径、`flush` 可解析并做类型检查），不引入新的文件对象体系。

## 3. 现状差异（代码锚点）

* `print` 内建当前直接走 `PyObject::Print`，且接受 `eol`：`src/builtins/builtins-io.cc`

* `print` 仍是 vtable internal slot，且代码里已有“需移除”注释：`src/objects/klass-vtable.h`

* 分发入口仍存在：`PyObject::Print(...)`：`src/objects/py-object.h/.cc`

* 大量内建类型实现 `Virtual_Print`（list/dict/tuple/string/number/function/type/iterators/views 等）

## 4. 实施阶段

### 阶段 A：先补齐 str/repr 统一语义入口

1. 盘点并统一“对象转文本”公共路径：

   * 明确 `Runtime_NewStr` 的职责边界（`str(x)` 语义）；

   * 新增或完善 `Runtime_NewRepr`（`repr(x)` 语义），避免容器/print 各自重复拼接逻辑。
2. 统一 fallback 规则：

   * `str` 优先走 `__str__`，缺失时回退 `__repr__`；

   * `repr` 走 `__repr__`，缺失时使用 object 默认表示。

### 阶段 B：让内建类型与 trampolines 对齐

1. 把现有 `Virtual_Print` 逻辑迁移到 `Virtual_Str/Virtual_Repr`：

   * 标量：`int/float/bool/None`

   * 容器：`list/tuple/dict`

   * 运行时可见函数对象：`function/native-function/method`

   * 元对象：`type`

   * 迭代器与 dict-views 家族
2. 保证动态类重写 `__str__/__repr__` 时，vtable 覆盖仍走现有 trampoline 链路（`klass-vtable-trampolines.cc`）。
3. 在 builtins 类型方法安装层补齐必要暴露（如 `object.__str__/object.__repr__`），与 Python 层 introspection 行为一致。

### 阶段 C：重构 builtins.print

1. 参数解析改为 CPython 风格关键字集合：

   * 移除 `eol`

   * 保留并严格校验 `sep/end`

   * 预留 `file/flush` 最小兼容行为
2. 输出路径：

   * 对参数对象统一走 `str` 路径得到 `PyString` 后输出；

   * `sep/end` 同样按字符串值输出；

   * 保持异常传播为 MaybeHandle 约定。
3. 更新原有 `native-print-unittest` 中依赖 `eol` 的断言。

### 阶段 D：移除 print 虚方法体系

1. 删除 vtable 中 `print_` slot（定义、初始化、覆盖逻辑中的痕迹）。
2. 删除 `PyObject::Print` 声明与实现，以及所有调用点替换。
3. 删除各 `Virtual_Print` 函数与 `vtable_.print_ = ...` 赋值。
4. 做全仓检索确保无残留符号（`Print(`/`print_`/`Virtual_Print`）。

## 5. 测试与验证矩阵

## 5.1 单测扩展

1. `native-print-unittest.cc`

   * 覆盖：默认分隔/结尾、`sep/end`、`eol` 非法关键字报错；

   * 覆盖：`print(obj)` 使用 `__str__`；缺 `__str__` 时回退 `__repr__`。
2. 解释器语义测试（现有 interpreter 测试文件扩展或新增）

   * 覆盖：`repr(x)` builtin 可用；

   * 覆盖：`str(x)`/`repr(x)` 对同一对象的差异；

   * 覆盖：自定义类覆写 `__str__/__repr__` 的 trampoline 生效。
3. 若新增测试文件，更新 `test/unittests/BUILD.gn` 的 `sources`。

### 5.2 回归执行

1. 构建并运行 `ut` 全量或至少 interpreter + objects 相关子集。
2. 额外做符号残留检索：

   * `Virtual_Print`

   * `vtable().print_`

   * `PyObject::Print`
3. 抽样运行已有 print/构造器相关解释器单测，确认无行为回退。

## 6. 风险与应对

* 风险 1：容器文本表示迁移时，递归元素打印链路容易引入行为差异。\
  应对：先把“元素转文本”统一封装，再逐类型替换。

* 风险 2：`str/repr` fallback 与异常传播路径耦合。\
  应对：优先复用 runtime helper，所有调用点保持 `RETURN_ON_EXCEPTION` 宏约定。

* 风险 3：删除 vtable slot 影响面大。\
  应对：按“先引入新路径 -> 再替换调用 -> 最后删旧符号”三步走，期间保持可编译。

## 7. 交付判定标准（DoD）

1. 代码层面：对象系统中不再存在 `print` 虚方法槽与入口。
2. 语义层面：`print` 主路径按 `str` 输出，`repr`/`str` 能在内建对象与自定义类场景下正确工作。
3. 测试层面：新增/改造用例全部通过，且关键回归无退化。

