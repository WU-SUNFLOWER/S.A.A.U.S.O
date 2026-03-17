# Interpreter 单测目录说明文档

本文档用于说明 `test/unittests/interpreter` 目录下各 `*-unittest.cc` 的职责边界，便于新增测试时快速归位，避免测试文件继续膨胀。

此外，本文档还包括对 `test/unittests/interpreter` 目录下 `*-unittest.cc` 单测文件的命名规范，以及新增单元测试的名称命名规范。

## 文件职责映射

- `interpreter-attribute-constraints-unittest.cc`：内建对象属性约束、`__dict__` 可见性与优先级、相关访问边界。
- `interpreter-builtins-bootstrap-unittest.cc`：builtins 启动注册完整性、核心对象可用性、基础 `str/repr` 可用性。
- `interpreter-builtins-constructor-unittest.cc`：`int/float/str/list/tuple/type` 构造流程与参数边界。
- `interpreter-builtins-method-dispatch-unittest.cc`：内建方法分派（实例调用、类型对象调用、receiver 校验错误）。
- `interpreter-builtins-subclass-gc-unittest.cc`：`tuple/str` 子类化与 `sysgc` 压力下存活性。
- `interpreter-call-stack-unittest.cc`：调用栈路径与关键调用栈形态约束。
- `interpreter-closure-unittest.cc`：闭包、嵌套作用域与 free var 错误边界。
- `interpreter-controlflow-unittest.cc`：`if/while/break/continue` 控制流语义。
- `interpreter-custom-class-attribute-binding-unittest.cc`：自定义类属性绑定、方法绑定与 `__getattr__/__setattr__` 语义。
- `interpreter-custom-class-build-class-unittest.cc`：`__build_class__` 入口与错误边界。
- `interpreter-custom-class-builtin-bridge-unittest.cc`：自定义类与内建 `__new__/__init__` 桥接行为。
- `interpreter-custom-class-construct-unittest.cc`：`__new__/__init__` 协作、构造参数边界、MRO 构造链。
- `interpreter-custom-class-core-unittest.cc`：自定义类定义、实例化、继承与 `isinstance` 基础语义。
- `interpreter-custom-class-dunder-unittest.cc`：自定义类 dunder 运算/打印协议。
- `interpreter-custom-class-mro-unittest.cc`：C3 MRO、菱形继承、查找顺序与冲突边界。
- `interpreter-dict-unittest.cc`：dict 构造/读写/视图 API、异常传播、dict 子类与 GC 场景。
- `interpreter-exceptions-unittest.cc`：`try/except/finally/raise/from`、异常匹配与 unwind 行为。
- `interpreter-exec-unittest.cc`：`exec` 作用域语义与参数错误边界。
- `interpreter-functions-unittest.cc`：函数调用协议（位置/关键字/默认值/`*args/**kwargs`）与调用错误边界。
- `interpreter-import-unittest.cc`：导入系统（模块缓存、包、相对导入、`import *`、失败回退）。
- `interpreter-lambda-unittests.cc`：lambda 基础行为与调用语义。
- `interpreter-list-basic-unittest.cc`：list 基础构造/访问/迭代/变更与运算主流程语义。
- `interpreter-list-exceptions-unittest.cc`：list 异常传播与错误信息语义（当前聚焦 `index`）。
- `interpreter-list-sort-unittest.cc`：`list.sort` 主流程（默认、`reverse`、`key`、稳定性）与排序错误。
- `interpreter-list-subclass-unittest.cc`：list 子类化与自定义初始化语义。
- `interpreter-math-module-unittest.cc`：内建 `math` 模块导入与核心 API 边界。
- `interpreter-ops-unittest.cc`：比较、整除、`is`、`contains` 等基础运算语义。
- `interpreter-random-module-unittest.cc`：内建 `random` 模块导入与 API 边界。
- `interpreter-smoke-unittest.cc`：解释器最小冒烟路径与返回约定。
- `interpreter-str-unittest.cc`：`str` 的 `index/find/rfind/split/join` 功能与错误边界。
- `interpreter-time-module-unittest.cc`：内建 `time` 模块导入、核心 API 与边界。
- `interpreter-tuple-unittest.cc`：tuple 基础构造与 `index` 异常传播边界。
- `native-print-unittest.cc`：原生 `print` 的 `sep/end` 语义与参数约束。

## 新增单测归位规则

- list 基础行为（构造、访问、遍历、append/insert/pop/reverse、运算）放入 `interpreter-list-basic-unittest.cc`。
- list 排序行为（`sort` 及其参数、稳定性、排序相关报错）放入 `interpreter-list-sort-unittest.cc`。
- list 异常传播与错误信息（如 `index`/比较异常）放入 `interpreter-list-exceptions-unittest.cc`。
- list 子类化与自定义初始化行为放入 `interpreter-list-subclass-unittest.cc`。
- 其余测试按“最窄职责”优先放入对应专题文件；若无法归类且形成新专题，再新建单测文件并同步更新本 README。

## 命名规范

- 测试名统一采用 PascalCase，避免缩写与歧义词。
- 推荐结构：`<Type><Feature><Scenario><Expected>`。
- list 相关推荐结构：`List<Feature><Scenario><Expected>`。
- `Feature` 表示被测能力（如 `Sort`、`Index`、`Append`、`Subclass`）。
- `Scenario` 表示前置场景（如 `WithKey`、`OnMixedTypes`、`PropagatesExceptionFromEq`）。
- `Expected` 表示预期结果（如 `ReturnsSortedList`、`RaisesTypeError`、`ValueError`、`Succeeds`）。
- 若测试已足够清晰，可省略 `Scenario` 或 `Expected` 之一，但不建议两者都省略。

### 命名示例

- `ListSortWithKeyKeepsStability`
- `ListSortOnMixedTypesRaisesTypeError`
- `ListIndexTupleNotFoundValueError`
- `ListAppendMultipleTypesSucceeds`

## 单测文件命名规范

- 统一格式：`interpreter-<domain>-unittest.cc`。
- `domain` 使用小写 kebab-case，按“最窄职责”命名，避免 `misc`、`all`、`temp` 等模糊词。
- 若是同一类型下的二级专题，使用 `<type>-<topic>`，如 `list-sort`、`list-exceptions`、`list-subclass`。
- 若是跨类型横切专题，直接用能力域命名，如 `builtins-method-dispatch`、`custom-class-mro`。
- 新文件名应与职责一一对应：读文件名即可判断“应放哪些测试、不应放哪些测试”。
- 避免在文件名中编码“版本号/阶段号/人名/日期”，保持长期稳定。

### 文件命名示例

- `interpreter-list-basic-unittest.cc`
- `interpreter-list-sort-unittest.cc`
- `interpreter-list-exceptions-unittest.cc`
- `interpreter-list-subclass-unittest.cc`
- `interpreter-builtins-method-dispatch-unittest.cc`

