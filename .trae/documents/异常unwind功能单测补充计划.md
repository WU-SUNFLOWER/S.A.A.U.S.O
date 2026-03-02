# 异常 unwind 功能：单元测试补充计划（MVP）

## 结论：是否还需要继续补充单测

需要。当前异常相关用例已覆盖“同一栈帧内命中 handler / finally 正常路径 / return 触发 finally / except 匹配边界”，但对 **unwind（跨栈帧展开、销毁栈帧、继续向上查表）** 的验证仍然偏少；这类 bug 往往表现为：

* 异常传播到上层时丢失（突然被吞）

* 栈帧未正确销毁导致栈不平衡/悬垂引用

* handler 查表 pc 偏移导致“该捕获却 miss”

* finally 顺序错误（先外层 finally 再内层 finally 等）

因此建议补齐一组“专门打 unwind”的端到端解释器用例，并以 print 输出作为可观察行为，保证回归稳定。

## 目标与边界（MVP）

* 目标：充分验证解释器的异常展开路径 `pending_exception_unwind` + `UnwindCurrentFrameForException()` 在多层调用/多层 try/finally 下工作正常。

* 边界：不要求 traceback、异常消息内容、cause 链展示（Step 1 MVP 允许简化）。

* 约束：保持与现有 `BasicInterpreterTest` 风格一致（脚本 + print 捕获断言），避免引入新测试夹具。

## 计划：补充哪些单测（按优先级）

### A. 跨栈帧传播（最关键）

1. **跨函数捕获**

   * 结构：`def g(): raise ValueError; def f(): g(); try: f() except ValueError: print("caught")`

   * 断言：能捕获且程序继续执行。

   * 覆盖点：至少一次 `UnwindCurrentFrameForException()` 销毁 g/f 栈帧后，在上层查表命中 handler。

2. **跨函数 finally 执行顺序（内→外）**

   * 结构：`g` 内 finally 打印，`f` 内 finally 打印，最外层 except 捕获。

   * 断言：打印顺序应为 `g.finally` → `f.finally` → `caught` → `after`。

   * 覆盖点：异常传播过程中每层 finally 都应执行且顺序正确。

3. **跨函数 mismatch 继续传播**

   * 结构：内层 `except ValueError`，实际抛 `RuntimeError`，外层捕获 `RuntimeError`。

   * 断言：内层 except 不执行，但内层 finally 执行，最终外层捕获。

   * 覆盖点：不命中 handler 时必须继续向上展开，且中途的 finally 不可丢。

### B. handler 栈回退与栈平衡（高风险实现点）

1. **except 内部再次抛出新异常（替换异常）**

   * 结构：`except ValueError: print("except"); raise RuntimeError`，外层捕获 `RuntimeError`。

   * 断言：`except` 打印一次；最终捕获 RuntimeError；finally 顺序正确。

   * 覆盖点：进入 handler 后 operand stack 被截断到表项 stack-depth；否则容易残留脏栈导致后续指令读错。

2. **finally 内抛异常（覆盖原异常）**

   * 结构：`try: raise ValueError finally: raise RuntimeError`，外层捕获 `RuntimeError`。

   * 断言：最终异常为 RuntimeError 可捕获，且不会出现“先捕 ValueError”之类的错序。

   * 覆盖点：finally 的异常优先级与 pending\_exception\_ 覆盖逻辑。

### C. 控制流相关 unwind（可选，但很值）

1. **for-loop + break/continue + finally**

   * 结构：循环中 `try/finally`，在 try 里 `break/continue`，finally 打印。

   * 断言：finally 必执行，且循环行为符合预期。

   * 备注：如果当前字节码子集对该控制流支持不足，则此项可先标记为“预期失败/暂缓”，等控制流字节码补齐后再启用。

2. **return 覆盖异常（return in finally）**

   * 结构：`try: raise ValueError finally: return "ok"`，调用处 `print(f())`。

   * 断言：输出应为 `"ok"`（CPython 语义：finally 的 return 会吞掉异常）。

   * 备注：这会强约束你的控制流实现；如果 MVP 暂不追求此兼容性，可不做或先做为 disabled case。

## 实施方式（进入执行阶段后）

* 把 A/B 的 5 个用例优先落地到 `test/unittests/interpreter/interpreter-exceptions-unittest.cc`。

* 运行定向用例过滤器验证，再跑 `ut.exe` 全量回归。

* 若任一用例失败，按“先定位 unwind vs except-match vs finally 控制流”的顺序修复：

  * handler 查找：`pending_exception_pc_`、exception table 单位与范围判断

  * 栈平衡：handler `stack_depth` 截断、压栈顺序（lasti/exc）

  * 跨帧：`UnwindCurrentFrameForException()` 销毁/恢复 caller 状态是否正确

* 若上述单测均顺利通过，再进一步补充`C. 控制流相关 unwind相关单测并进行回归`

## 验收标准

* 新增用例全通过。

* `ut.exe` 全量通过。

* 不引入额外 `fprintf/exit` 路径或未捕获崩溃。

