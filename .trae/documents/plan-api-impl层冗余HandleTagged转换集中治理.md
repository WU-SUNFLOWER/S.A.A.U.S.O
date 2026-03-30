# API 实现层冗余 Handle/Tagged 转换集中治理计划

## Summary

* 目标：在不改变 Embedder 公共 API 语义、不新增过渡 API 的前提下，集中清理 `src/api/` 中“可直接证明冗余”的 Handle/Tagged/向上向下转换样板，提升可读性与一致性。

* 已确认用户给出的两个观察都成立：

  * `src/api/api-impl.h` 中 `scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*py_string), isolate))` 可直接收敛为 `scope.Escape(py_string)`，再依赖现有 `Handle<Derived> -> Handle<Base>` 隐式上转赋给 `Handle<PyObject>`。

  * `src/api/api-objects.cc` 中 `i::PyObject::Call(..., i::handle(*py_args, internal_isolate), i::handle(*py_kwargs, internal_isolate))` 可直接传 `py_args` 与 `py_kwargs`，依赖现有 `Handle<PyTuple>/Handle<PyDict> -> Handle<PyObject>` 隐式上转。

* 本轮采用“保守收敛”策略：优先复用现有能力（隐式 Handle 上转、`Handle<T>::cast`），不引入新的 helper，不扩展到 `reinterpret_cast<i::Isolate*>` 这类 API/内部边界桥接。

## Current State Analysis

### 已核实的现象

* `src/api/api-impl.h`

  * `WrapHostString` 与 `WrapHostFloat` 先持有 `Handle<PyString>/Handle<PyFloat>`，再降为 `Tagged<PyObject>`，随后又重新提升为 `Handle<PyObject>` 后 `Escape`，属于明显冗余。

  * `WrapHostBoolean` 中也存在把已可表达为 `Handle<PyObject>` 的值写成更冗长 Tagged 转换链的情况。

* `src/api/api-objects.cc`

  * `Object::Get` 对 `py_key` 的调用参数构造存在不必要的 `Tagged<PyObject>` 回绕。

  * `Object::CallMethod` 对 `py_name`、`py_args`、`py_kwargs` 的传参与用户观察同型，属于“已有 Handle 却再次拆回 Tagged 再包回 Handle”的样板。

* `src/api/api-container.cc`

  * `List::New`、`Tuple::New` 的返回值包装存在与 `WrapHostString` 同型的“Handle<Derived> -> Tagged<PyObject> -> Handle<PyObject> -> Escape”。

  * `List::Push`、`List::Set`、`List::Get`、`Tuple::Get` 中存在“`Handle<PyObject>` 已完成类型检查后，仍手写 `Tagged::cast + handle(...)` 恢复为具体 Handle”的样板，可优先改为 `Handle<T>::cast(object)` 或直接复用已有 Handle。

* `src/api/api-context.cc`

  * `Context::Set/Get` 中 `context_object` 与 `py_key` 的具体类型恢复均存在同类冗余，可改为 `Handle<PyDict>::cast(context_object)` 与直接传 `py_key`。

* `src/api/api-script.cc`

  * `Script::Run` 中 `context_object` 已完成 `IsPyDict` 判定，恢复为 `Handle<PyDict>` 时可直接 `Handle<PyDict>::cast(context_object)`，无需 `Tagged::cast + handle(...)`。

### 不纳入本轮的内容

* `internal::Utils::OpenHandle(...)`、`ToLocal(...)`、`reinterpret_cast<i::Isolate*>` 这类 API/内部边界桥接仍是当前 Embedder API 设计的一部分，不属于本轮“可直接删除的冗余转换”。

* `api-value.cc`、`api-primitive.cc` 中仅用于读取原始字段值的 `Tagged::cast(*object)` 暂不处理；这些调用不是“Handle 与 Tagged 来回绕行”，而是直接读取对象布局。

* 不新增内部 helper，不修改公共头文件 ABI，不改变异常传播、HandleScope 生命周期与 isolate 选择策略。

## Proposed Changes

### 1. 收敛 `api-impl.h` 的宿主值包装样板

* 文件：[api-impl.h](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-impl.h)

* 改动：

  * `WrapHostString`：直接 `Escape(py_string)`。

  * `WrapHostFloat`：直接 `Escape(py_float)`。

  * `WrapHostBoolean`：改为更直接的 `Handle<PyObject>` 构造方式，避免冗长 Tagged 显式转换链。

  * `WrapHostInteger` 保持当前 `PySmi::FromInt` 路径，但若存在无需 isolate 的更短既有写法，仅在不改变语义时顺手统一。

* 原因：这些函数是 API 层样板的源头，先收敛此处可统一风格并为后续返回值包装提供范式。

### 2. 收敛容器与对象 API 中的具体类型恢复写法

* 文件：[api-container.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-container.cc)

* 改动：

  * `List::New`、`Tuple::New`：直接 `Escape(list/tuple)`。

  * `List::Push`、`List::Set`、`List::Get`、`Tuple::Get`：在完成 `IsPyList/IsPyTuple` 检查后，优先使用 `Handle<PyList>::cast(object)`、`Handle<PyTuple>::cast(object)`，移除多余 `Tagged::cast + handle(...)`。

* 文件：[api-objects.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-objects.cc)

* 改动：

  * `Object::Get`：直接传 `py_key` 给 `dict->Get(...)`。

  * `Object::CallMethod`：直接传 `py_name`、`py_args`、`py_kwargs`，移除不必要 `i::handle(*..., isolate)`。

  * 保持 `Handle<i::PyDict>::cast(object)` 的既有简洁写法，并向同类代码对齐。

* 文件：[api-context.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-context.cc)

* 改动：

  * `Context::Set/Get`：对 `context_object` 统一使用 `Handle<PyDict>::cast(...)`；对 `py_key` 直接按 `Handle<PyString>` 参与调用，由隐式上转承担参数匹配。

* 文件：[api-script.cc](file:///e:/MyProject/S.A.A.U.S.O/src/api/api-script.cc)

* 改动：

  * `Script::Run`：把 `context_object` 的具体类型恢复改为 `Handle<PyDict>::cast(context_object)`，消除多余 Tagged 往返。

* 原因：这些文件是当前冗余转换最集中的热点，且都可在不改变语义的前提下通过现有类型系统直接化简。

### 3. 统一风格并做一次 API 层残留复查

* 范围：`src/api/*.cc`、`src/api/api-impl.h`

* 改动：

  * 复查是否仍残留以下同型样板，并按“仅在语义等价且更短时”继续清理：

    * `scope.Escape(i::handle(i::Tagged<i::PyObject>::cast(*x), isolate))`

    * `Foo(..., i::handle(*x, isolate))`，其中 `x` 已是 `Handle<PyString/PyTuple/PyDict/...>`

    * `i::handle(i::Tagged<Concrete>::cast(*object), isolate)`，其中 `object` 已是完成类型检查的 `Handle<PyObject>`

  * 不追求把所有 `Tagged::cast` 清零，只处理“明显是 Handle/Tagged 往返样板”的位置。

* 原因：保证这轮集中治理不是只修两个点，而是形成一致的 API 层写法。

## Assumptions & Decisions

* 决策：本轮采用“保守收敛 + 优先现有能力”的方案，不新增 helper，不引入过渡 API。

* 决策：允许依赖当前 `Handle` 模板的现有能力：

  * `Handle<Derived>` 到 `Handle<Base>` 的隐式向上转换。

  * `Handle<T>::cast(Handle<U>)` 在已完成类型检查时恢复具体类型。

* 假设：本轮清理只改变表达方式，不改变：

  * 返回值生命周期与 `EscapableHandleScope::Escape` 行为；

  * 异常传播与 `MaybeLocal/MaybeHandle` 约定；

  * 公共 API 的 ABI 与 include 边界。

* 决策：若某处化简会削弱调试期断言、改变 isolate 显式性、或触发模板匹配歧义，则保留原写法，不为“极限简短”牺牲稳健性。

## Verification

* 编译验证：

  * 构建 `embedder_api` 所在目标，确认 API 层模板与调用签名收敛后无编译错误。

* 测试验证：

  * 运行 `test/embedder:embedder_ut`，重点覆盖 `Context/Object/List/Tuple/Script` 相关路径。

  * 如构建成本可接受，再补跑根 `ut` 目标，确认未引入全局回归。

* 代码审阅验证：

  * 对比治理前后，确认所有修改都属于语义等价的“删除冗余桥接”，未混入行为变更。

  * 重点复核用户给出的两个示例位置，确保最终代码确实按预期化简。

