# PyDict 异常传播改造方案

## 背景与问题

当前 `PyDict::Put/Get/GetTagged/Contains/Remove` 在内部调用 `PyObject::Hash/Equal`（均可能失败并设置 pending exception）时，普遍采用“返回 null / 直接 return”的方式结束流程：

* `Get/GetTagged`：失败时返回 `null`，与“未命中”语义混淆。

* `Put/Remove`：失败时 `return`，调用方无法感知写入/删除是否真正完成。

* `Contains`：返回 `bool`，会把“异常失败”误当成“未包含”。

这会造成上层（runtime / builtins / interpreter）在未显式检查 pending exception 时，可能继续执行并抛出错误类型（如把 `__eq__` 抛出的异常变成 `KeyError/NameError`），或在 pending exception 状态下继续写入/构造对象，导致语义与控制流不一致。

## 目标

1. 让 dict 的基础操作具备“可传播异常”的 API，能区分：

   * 操作成功且命中/未命中

   * 操作失败（pending exception）
2. 迁移关键调用点，保证 pending exception 一旦出现立刻向上传播，避免“错误覆盖/吞掉”。
3. 收敛解释器侧对 dict 细节的直接依赖：优先通过 runtime 入口承接字节码/内建语义，objects 层仅提供可组合的原语操作。

## 总体策略（分层职责）

* **objects（`PyDict`）**：提供“可失败”的原语操作（查找/插入/删除/扩容），不决定抛什么 Python 异常类型。

* **runtime**：把原语组合为 Python 语义（例如 `d[key]` 未命中抛 `KeyError`、合并策略、kwargs 更新等），并统一使用 exception-utils 宏传播异常。

* **interpreter**：尽量只调用 runtime 的语义入口；若仍需直接访问 `PyDict`，必须使用 fallible API 并在失败时立即跳转到异常展开。

## API 改造设计

### 新增 fallible API（不立即破坏旧 API）

在 `PyDict` 增加以下接口（命名可在实现时按仓库风格微调）：

* `Maybe<Tagged<PyObject>> GetTaggedMaybe(Tagged<PyObject> key) const`

  * `Nothing()`：内部 `Hash/Equal` 失败（pending exception 已设置）。

  * `Just(value)`：查找成功完成；`value` 可能为 `Tagged<PyObject>::null()` 表示未命中。

* `Maybe<bool> Put(Handle<PyObject> key, Handle<PyObject> value)`

  * `Nothing()`：`Hash/Equal/Expand` 失败。

  * `Just(true/false)`：成功；`true` 表示插入新 key，`false` 表示覆盖已有 key（仅用于调试/统计；大多数调用点可忽略返回值）。

* `Maybe<bool> Remove(Handle<PyObject> key)`

  * `Nothing()`：`Hash/Equal` 失败。

  * `Just(true/false)`：成功；`true` 表示删除成功，`false` 表示 key 不存在。

* `Maybe<bool> ContainsKey(Handle<PyObject> key) const`

  * `Nothing()`：失败。

  * `Just(true/false)`：成功。

* 将 `ExpandImpl` 拆为 `Maybe<void>`-风格的可失败实现不可行（当前 `Maybe<void>` 不存在），因此建议：

  * `Maybe<bool> ExpandIfNeeded()` / `Maybe<bool> ExpandImplMaybe()`：用 `Maybe<bool>` 表示“扩容是否执行/是否成功完成”，`Nothing()` 表示失败。

说明：`MaybeHandle<T>` 当前以 “location 是否为空” 表示失败，无法区分“成功但结果 handle 为 null”与“失败”，因此 dict 的三态结果不适合用 `MaybeHandle` 表达，应使用 `Maybe<T>`。

### 旧 API 的处置

保留现有：

* `Handle<PyObject> Get(...) const`

* `Tagged<PyObject> GetTagged(...) const`

* `static void Put(...)`

* `bool Contains(...) const`

* `void Remove(...)`

并将其实现改为调用新 fallible API，在失败时维持现有行为（返回 null / 直接 return），以便分阶段迁移调用点；同时把“正确传播异常”的责任逐步迁移到调用点（runtime/builtins/interpreter）。

## 调用点迁移清单（必须改）

### interpreter

目标：保证 dict 查找/写入一旦触发 pending exception，立即进入 `pending_exception_unwind`。

* `LoadName`：

  * 将 `locals/globals/builtins` 的 `GetTagged` 调用改为 fallible 版本；

  * 若 `Maybe` 为空则直接跳异常展开；仅在“成功完成且返回 null（未命中）”时继续下一个 dict；

  * 这样避免将 `__eq__`/`__hash__` 的异常错误地转换成 `NameError`。

* `StoreGlobal`、`BuildMap`：

  * `Put` 改用 fallible 版本并在失败时跳异常展开（避免写入失败但解释器继续执行）。

### objects / builtins

目标：字典对象的虚方法实现与 dict 内建方法要遵循异常传播。

* `PyDictKlass::Virtual_Subscr`：

  * `Get` 改为 fallible；失败直接返回 `kNullMaybeHandle`（异常已在 isolate），不要额外抛 `KeyError` 覆盖原异常。

* `PyDictKlass::Virtual_StoreSubscr`：

  * `Put` 改为 fallible；失败返回 `kNullMaybeHandle`。

* `PyDictKlass::Virtual_DeleteSubscr`：

  * `Contains/Remove` 改为 fallible；失败立刻返回 `kNullMaybeHandle`；

  * 仅在“成功且不包含”时抛 `KeyError`。

* `builtins-py-dict-methods.cc`（`get/pop/setdefault/...`）：

  * 所有 `Get/Put/Remove/Contains` 改为 fallible 并传播异常；

  * 确保不会把 `__eq__` 的异常变成 `None` 返回或 `KeyError`。

### runtime

目标：runtime 作为解释器友好 API 的收敛层，减少 interpreter 直接摸对象细节。

* `Runtime_NewDict/Runtime_MergeDict`：

  * 内部 `PyDict::Put/Contains` 改为 fallible；

  * 失败使用 `RETURN_ON_EXCEPTION`/`ASSIGN_RETURN_ON_EXCEPTION` 立即向上传播。

* 视情况新增/补齐 dict 语义入口（可选，但推荐）：

  * `Runtime_DictGetItem(dict, key)`：实现 `d[key]` 语义（未命中抛 `KeyError`，内部使用 fallible 查找）。

  * `Runtime_DictSetItem(dict, key, value)`：实现 `d[key]=v` 语义（内部使用 fallible 插入）。

  * 后续可把 `PyDictKlass::Virtual_*Subscr` 改为直接调用 runtime 入口，进一步集中语义。

## 代码结构改造建议（实现层面）

在 `py-dict.cc` 中抽出共享内部逻辑，避免多处重复探测循环：

* `Maybe<uint64_t> HashKey(Handle<PyObject> key)`（或直接内联调用 `PyObject::Hash`，取决于可读性）

* `Maybe<bool> KeysEqual(Tagged<PyObject> stored_key, Tagged<PyObject> lookup_key)`

* `Maybe<int64_t> FindSlot(Tagged<PyObject> key, bool* found)`

  * 统一处理 `Hash`、线性探测、`Equal`；

  * found=true 时返回槽位；found=false 时返回空槽位（用于插入）。

* `Maybe<Tagged<PyObject>> GetTaggedMaybe`、`Maybe<bool> Put`、`Maybe<bool> Remove` 基于 `FindSlot` 实现。

扩容：

* `Put` 在扩容前后都要确保“失败则不继续写入”，并保证字典状态不被半更新。

## 测试与验证

新增/补齐单测，覆盖“异常不被覆盖/不被吞掉”的关键路径：

1. `__hash__` 抛异常：

   * `d = {}; d[BadHash()] = 1` 应抛出 `ValueError`（或对应异常），不能静默成功。
2. `__eq__` 抛异常（可控碰撞）：

   * `A.__hash__ -> 0, A.__eq__ raise`

   * `B.__hash__ -> 0`

   * `d = {A(): 1}; d[B()]` 应抛出 `ValueError`，不能变成 `KeyError`/返回 `None`。
3. 删除/contains 路径：

   * `pop`/`del` 在 `__hash__` 或 `__eq__` 抛异常时应传播原异常。
4. 解释器字节码路径：

   * `BUILD_MAP` / `STORE_GLOBAL` 写入异常时应触发解释器异常展开而不是继续执行（可用脚本断言后续语句不执行）。

测试放置位置：

* 若已有解释器端到端测试框架（如 `BasicInterpreterTest`），优先新增脚本级用例并复用统一的异常断言 helper。

* 若 dict 方法测试分散在 builtins/objects 单测，按现有组织方式补充。

## 迁移步骤（落地顺序）

1. 在 `PyDict` 增加 fallible API，并用内部共享逻辑实现（不改调用点）。
2. 迁移 `PyDictKlass` 与 `builtins-py-dict-methods` 到 fallible API，修复 `KeyError/None` 覆盖真实异常的问题。
3. 迁移 runtime（`Runtime_NewDict/Runtime_MergeDict`）到 fallible API，保证 kwargs/merge 语义在异常时停止。
4. 迁移 interpreter 的 `LoadName/StoreGlobal/BuildMap`，确保 pending exception 立即展开。
5. 补齐单测并跑全量测试，确认无行为回归。

## 风险与兼容性

* 兼容性：保留旧 API 以降低一次性改动面，但会在迁移期间同时存在两套 API；需要在 code review 中强制新代码使用 fallible 版本。

* 性能：fallible API 增加了少量分支与 `Maybe` 构造；关键路径（如 name lookup）可在后续引入“字符串 key 快路径”优化，但优先保证语义正确。

