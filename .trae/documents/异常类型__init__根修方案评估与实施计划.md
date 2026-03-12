# 异常类型 `__init__` 根修方案评估与实施计划

## 1. 问题与根因结论

* 现象：`ValueError("from iterator")` 触发 `TypeError: ValueError.__init__() takes exactly one argument`。

* 根因：异常层级未实现并挂载 `BaseException.__init__`，导致 `ValueError/Exception` 继承到 `object.__init__`；当传入消息参数时被 `object` 规则拒绝。

* 结论：需要补齐异常体系的初始化语义，但应在 `BaseException` 统一实现，而不是为每个异常子类分别补 `__init__`。

## 2. 修复目标

* 让 `BaseException(*args, **kwargs)` 接收异常构造参数并完成实例状态初始化。

* 让所有异常子类（`Exception/ValueError/StopIteration/...`）通过继承自动获得正确 `__init__` 行为。

* 消除“脚本抛错路径（`raise ValueError("x")`）”与“运行时抛错路径（`Runtime_ThrowError*`）”在异常实例字段上的语义分叉。

## 3. 具体实施步骤

1. **新增** **`BaseException.__init__`** **内建方法入口**

   * 文件：`src/builtins/builtins-base-exception-methods.h/.cc`

   * 动作：

     * 将 `Init` 加入 `BASE_EXCEPTION_BUILTINS` 安装列表，使其在 `BaseExceptionMethods::Install` 中自动注入类字典。

     * 新增 `BUILTIN_METHOD(BaseExceptionMethods, Init)` 实现。

2. **定义** **`BaseException.__init__`** **的最小正确语义**

   * 文件：`src/builtins/builtins-base-exception-methods.cc`

   * 语义约束：

     * `self` 必须非空，否则抛 `TypeError`。

     * 接受任意位置参数 `*args`。

     * 关键字参数策略：MVP 阶段优先与当前对象模型保持一致，若暂不支持命名参数则在 `kwargs` 非空时抛 `TypeError`（错误信息对齐 `BaseException.__init__() takes no keyword arguments`）。

   * 状态写入：

     * 将 `args`（或其克隆）写入实例属性 `args`。

     * 将 `message` 作为兼容字段维护：

       * `len(args) == 0`：写空字符串；

       * `len(args) >= 1`：首参若为 `str` 则直接使用；否则走 `str(first_arg)` 的可回退转换路径。

     * 返回 `None`，满足 `InitInstance` 的返回约束。

3. **收敛异常文本读取口径（保证向后兼容）**

   * 文件：`src/builtins/builtins-base-exception-methods.cc`、`src/runtime/runtime-exceptions.cc`

   * 动作：

     * `BaseException.__str__` 优先读取 `args` 生成文案；若不存在再回退 `message`（兼容旧实例）。

     * `Runtime_FormatPendingExceptionForStderr` 同步采用“优先 `args`、回退 `message`”口径，避免不同打印路径出现文本不一致。

4. **收敛运行时抛错构造路径（可选增强，建议同次落地）**

   * 文件：`src/runtime/runtime-exceptions.cc`

   * 动作：

     * `Runtime_NewExceptionInstance` 在 `message_or_null` 非空时，优先通过统一异常初始化语义构造 `args/message`（避免仅写 `message`）。

     * 在不破坏现有行为前提下保留 `message` 兼容写入，逐步过渡到以 `args` 为主。

5. **确认初始化链路无需对子类单独改造**

   * 文件：`src/interpreter/builtin-bootstrapper.cc`

   * 动作：

     * 复核 `BaseException` 方法注入时序位于派生异常类型注册前，确保 `Exception/ValueError` 自动继承 `__init__`。

     * 不给 `ValueError` 单独挂 `__init__`，避免重复实现和后续维护分叉。

## 4. 验证计划

* 用 `src/main.cc` 当前样例回归：

  * 期望输出流程为 `1`、`caught`、`after`，不再出现 `ValueError.__init__` 参数错误。

* 新增/补充解释器层异常相关用例（沿用现有异常断言 helper）：

  * `raise ValueError("x")`：异常类型与文本正确。

  * `raise Exception()`：空消息路径正确。

  * `BaseException.__init__(obj, key=1)`：触发关键字参数错误（若采用 no-kwargs 策略）。

  * `str(ValueError("x"))` 与 stderr 格式化结果一致。

## 5. 风险与控制

* 风险：`message` 与 `args` 双轨并存期间可能出现读取口径不一致。

* 控制：统一“优先 `args`、回退 `message`”策略，并保持 `message` 兼容写入直到历史路径完全收敛。

* 风险：`str(first_arg)` 转换可能触发二次异常。

* 控制：严格使用现有 `MaybeHandle`/`RETURN_ON_EXCEPTION` 传播，确保 pending exception 语义不被吞掉。

## 6. 本次评估回答（针对提问）

* 是否需要补 `__init__`：**需要**。

* 是否为每个异常类型单独补：**不需要**。

* 根本修复方式：在 `BaseException` 统一实现 `__init__` 与参数语义，让全部异常子类继承该行为，并同步收敛异常打印/格式化对 `args` 的读取口径。

