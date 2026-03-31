## Summary

* 目标：利用 `PyList::PopTagged/GetTagged/GetLastTagged` 清理 VM 内部与单测中“不必要的先创建 Handle、再立刻解引用为 Tagged”的读路径，同时保证不会把裸 `Tagged<PyObject>` 跨越任何可能触发 GC/异常/分配的调用点长期持有。

* 范围：优先治理 `src/builtins/`、`src/runtime/`、`src/objects/` 与 `test/unittests/` 中真实存在的 `PyList` 热点读取循环和断言路径；不扩展到新的公开 API，也不顺手改动无收益的调用点。

## Current State Analysis

* `PyList` 已在 [src/objects/py-list.h](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.h#L29-L36) 与 [src/objects/py-list.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list.cc#L35-L60) 提供：

  * `PopTagged()`

  * `GetTagged(int64_t)`

  * `GetLastTagged()`

* 当前 Tagged 读取 API 仍几乎未被消费，主要调用方仍使用 `Get(..., isolate)`/`Pop(isolate)`。

* 明确存在“创建 Handle 后立即解引用为 Tagged”的热点：

  * [builtins-py-list-methods.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-py-list-methods.cc#L296-L301)：`sort()` 中 `keys->Set(i, *list->Get(i, isolate));`

  * [builtins-py-list-methods.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-py-list-methods.cc#L365-L368)：`sort()` 中 `tmp->Set(i, *list->Get(..., isolate));`

  * [runtime-iterable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-iterable.cc#L69-L87)：`list -> tuple` 拷贝路径仍走 `Get(..., isolate)` 后写入 `SetInternal`

* 另有若干 `PyList` 读路径虽可机械替换，但后续会立即进入可能触发 GC/异常/分配的调用，因此不能把裸 `Tagged<PyObject>` 跨调用点保存：

  * [py-list-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-klass.cc#L199-L245) 中 `Virtual_Add/Virtual_Mul` 会调用 `PyList::Append`

  * [py-list-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-klass.cc#L296-L376) 中比较/contains/equal 会调用 `PyObject::LessBool/GreaterBool/EqualBool`

  * [klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc#L27-L103) 中 C3 merge 会调用 `Remove/Append/Insert`

  * [py-list-iterator-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator-klass.cc#L29-L45) 返回路径需要 `Handle`

* 现有单测尚未直接覆盖 `PyList::GetTagged/PopTagged/GetLastTagged` 的行为，也没有专门验证本轮“只在 GC 安全窗口内消费 Tagged”这一使用边界。

## Proposed Changes

### 1. 收敛 `PyList` 的直接 Tagged 读路径热点

* 修改 [src/builtins/builtins-py-list-methods.cc](file:///e:/MyProject/S.A.A.U.S.O/src/builtins/builtins-py-list-methods.cc)

  * 在 `sort()` 的两个纯拷贝循环中，直接改用 `list->GetTagged(...)` 写入 `FixedArray`。

  * 若 `pop()` 的返回路径仍存在可读性更好的 Tagged 入口，可一并改为 `PopTagged()` 后在返回边界构造 `Handle`，但仅在不增加额外复杂度时处理。

  * 不改 `sort()` 中 `key_func` 调用前的 `elem` 读取策略，因为元素在进入 `Execution::Call(...)` 前必须以 `Handle` 形式被根化，不能持有裸 `Tagged` 跨调用。

* 修改 [src/runtime/runtime-iterable.cc](file:///e:/MyProject/S.A.A.U.S.O/src/runtime/runtime-iterable.cc)

  * 在 `Runtime_UnpackIterableObjectToTuple()` 的 `list -> tuple` 快路径中，改为 `tuple->SetInternal(i, list->GetTagged(i))`。

  * 保持 `Runtime_ExtendListByItratableObject()` 的 `list -> list` 展开逻辑继续使用 `Handle` 读取，因为 `PyList::Append()` 可能扩容并触发分配。

### 2. 审慎治理仅在“立即消费”场景成立的对象层调用点

* 修改 [src/objects/py-list-iterator-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-iterator-klass.cc)

  * 评估 `NextImpl()` 是否改为 `GetTagged(iter_cnt)` 并在返回边界立即 `handle(...)`。

  * 仅在实现更直接、且不会让裸 `Tagged` 跨 `iterator->increase_iter_cnt()` 之外的潜在分配点泄漏时采用。

* 修改 [src/objects/py-list-klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/py-list-klass.cc)

  * 逐个审视 `Virtual_Subscr`、`Virtual_Less`、`Virtual_Contains`、`Virtual_Equal` 等读路径。

  * 仅替换那些“读取后立即返回 / 立即转存”的位置；凡是后续要进入 `PyObject::*Bool`、`PyList::Append` 等可能触发 GC 的逻辑，继续保留 `Handle` 读法，避免伪优化。

* 修改 [src/objects/klass.cc](file:///e:/MyProject/S.A.A.U.S.O/src/objects/klass.cc)

  * 仅在 C3 线性化的“纯复制到容器槽位”位置考虑使用 `GetTagged`；涉及 `Remove/Insert/Append` 的位置保持 `Handle` 读取。

### 3. 补齐针对新 Tagged API 与 GC 边界的单测

* 修改 [test/unittests/objects/py-list-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/objects/py-list-unittest.cc)

  * 扩展现有基础用例，显式覆盖：

    * `GetTagged()` 返回的元素与 `Get()` 一致

    * `GetLastTagged()` 返回尾元素

    * `PopTagged()` 弹出元素后长度正确

  * 优先直接比对 `Tagged` 指针值 / Smi 解码值，避免继续用“先转 Handle 再解引用”的旧断言风格。

* 修改 [test/unittests/heap/gc-unittest.cc](file:///e:/MyProject/S.A.A.U.S.O/test/unittests/heap/gc-unittest.cc)

  * 补一条或调整现有 list 相关 GC 回归，使其覆盖“minor GC 后通过 `GetTagged()` 重新读取元素仍正确”的场景。

  * 不编写“缓存裸 Tagged 后再触发 GC 仍要求可用”的错误用例；该行为本身违反句柄/GC 约束。

## Assumptions & Decisions

* 决策：本轮治理的核心目标是**消除无意义的 Handle 包装/解包**，不是把所有 `PyList::Get()` 调用一律替换成 `GetTagged()`。

* 决策：凡是满足以下任一条件的调用点，继续保留 `Handle` 读取：

  * 读取结果要跨越可能触发分配、异常或 Python 调用的边界

  * 读取结果要在循环体中被缓存并在后续调用后继续复用

  * 读取结果本身就是 `MaybeHandle`/`Handle` API 的稳定输入

* 决策：允许在“函数返回边界”或“立即写入 `FixedArray/Tuple` 槽位”这种无 GC 窗口的场景直接使用 `GetTagged/PopTagged/GetLastTagged`。

* 假设：本轮不新增 `PyList::SetTagged` / `AppendTagged` 之类扩展 API，避免把任务扩大为新的容器接口设计。

* 假设：不新增测试文件；仅在现有 `py-list-unittest.cc` 与 `gc-unittest.cc` 中补充覆盖，因此无需修改 `test/unittests/BUILD.gn`。

## Verification

* 构建并运行与本轮修改直接相关的单测目标，至少覆盖：

  * `test/unittests/objects/py-list-unittest.cc`

  * `test/unittests/heap/gc-unittest.cc`

* 进行一轮全量或最小充分集回归，优先使用现有 `ut` 目标，确认：

  * 编译无新增警告/错误

  * `PyList` 相关对象层、builtins 层与 GC 测试全部通过

* 重点人工检查以下 GC 安全不变量：

  * 没有把 `Tagged<PyObject>` 保存到会跨越 `PyList::Append`、`Execution::Call`、`PyObject::*Bool`、工厂分配或异常抛出的调用之后继续使用

  * 所有跨 GC 边界继续存活的对象都由 `Handle` 持有

  * 新改成 `GetTagged()` 的位置都属于“立即消费、无分配窗口”的纯读取/转存场景

