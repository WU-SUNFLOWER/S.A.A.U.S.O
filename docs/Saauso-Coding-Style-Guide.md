# S.A.A.U.S.O Coding Style Guide

本文档定义 S.A.A.U.S.O 的 C++ 代码风格与注释规范，适用于人类程序员与 AI 助手。

规范来源以 [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) 为基础，并叠加本仓库的工程约束与可维护性要求。

## 1. AI 自检清单（提交前必过）

- **目标**：避免“上帝函数”和“无注释代码”，让 Reviewer 能快速判断正确性与可维护性。
- **函数边界**
  - **单一职责**：一个函数只负责一个清晰的任务；如果同时处理“解析/校验/分配/执行/清理”等多个阶段，必须拆分为多个小函数。
  - **长度触发器**：当函数主体 **超过约 40 行**时必须评估拆分（除非是结构性代码且已分段清晰说明）。
  - **复杂度触发器**：出现 **3 层及以上嵌套**、长链式条件、或需要读者在脑中维护多个状态变量时，必须优先提取辅助函数或引入局部小结构体承载中间结果。
- **禁止 magic number（可读性红线）**
  - **禁止**：在实现中直接出现意义不明的数字常量（magic number），尤其是位掩码、移位位数、协议常量、单位换算系数、状态值等。
  - **必须**：使用匿名命名空间内的 `constexpr` 常量（`kPascalCase`）并尽量缩小作用域，或使用 `enum class` 表达状态/分类型数值。
  - **允许的例外**：`0/1` 仅用于循环边界、数组下标、或明显无歧义的布尔转整型；一旦数字承载“协议/布局/单位/位语义”，必须命名化。
  - **示例**：
    ```cpp
    // Bad: 63/64/6/2 的语义不透明。
    val = b & 63;
    while (b & 64) { val <<= 6; }
    start *= 2;

    // Good: 用命名常量与 helper 表达协议/单位语义，且常量限制在匿名命名空间。
    constexpr uint8_t kVarintPayloadMask = 0x3F;
    constexpr uint8_t kVarintContinueMask = 0x40;
    constexpr int kVarintPayloadBits = 6;
    constexpr int kCodeUnitSizeInBytes = 2;
    ```
- **注释最低要求**
  - **新增/修改的对外接口**（头文件中声明的类/函数/方法）：声明处必须有中文注释，说明用途、关键参数语义（含是否允许 null）、返回值与关键约束。
  - **仅在 `.cc` 内部存在的 helper**（匿名命名空间/`static` 函数等）：定义上方必须有中文注释，解释其职责与输入输出。
  - **非显而易见逻辑**：在实现处用中文注释解释“为什么这么做/边界条件/不变量”，而不是重复代码字面含义。

## 2. 代码文件规范

- **文件**
  - C++ 源文件：`*.cc`
  - C++ 头文件：`*.h`
  - 文件名：`kebab-case`（例如 `py-code-object.cc`）
- **头文件依赖**
  - 为降低编译期依赖复杂度，减少编译/链接错误，优先使用前置声明。
  - 仅在需要完整类型（例如按值成员、继承、`sizeof`/`alignof`、模板/内联实现）时才 `#include`。

## 3. 整体代码风格（General Style）

- **命名空间**：所有内部代码必须位于 `namespace saauso::internal` 中。
- **通用命名规则**
  - 名称应该是描述性的（Descriptive）。例如 `int num_errors;`、`int num_dns_connections;`。
  - 严禁使用非通用缩写。允许使用业界通用缩写（如 `cpu/gc/vm/ir/abi/utf8`、`dns`、`url`）与仓库既有缩写。不要通过删除单词中的字母来缩写（例如严禁使用 `cstmr_id` 代替 `customer_id`）。
- **类**：`PascalCase`。
- **变量**：`snake_case`，类成员变量以 `_` 结尾。
- **常量**：`kPascalCase`（例如 `constexpr int kDaysInAWeek = 7;`、`const std::string kAndroid8_0 = "Android 8.0";`）
- **枚举值**：`kCamelCase`，例子：
  ```cpp
  enum UrlTableErrors {
    kOk = 0,
    kOutOfMemory,
    kMalformedInput,
  };
  ```
- **宏**：`MACRO_CASE`。在头文件中定义宏需要非常谨慎，应保证命名独特，以免污染全局命名空间。
- **类/结构体声明（头文件）**
  1. 访问控制段按 `public`→`protected`→`private`
  2. 数据成员集中放在末尾并尽量为 `private`（除非确有继承扩展需要）
  3. 每段内部按以下顺序排列：
     - 类型与类型别名（例如 typedef、using、enum、嵌套 struct/class、友元类型）
     - 静态常量
     - 工厂函数
     - 构造函数与赋值运算符
     - 析构函数
     - 其他所有函数（静态与非静态成员函数，以及友元函数）
     - 其他所有数据成员（静态与非静态）

## 4. 函数/方法风格（重要，严禁写超长的复杂函数）

- **命名规则**：`PascalCase`（例如 `InitMetaArea()`、`IterateRoots()`、`RecordWrite()`）。
  - 例外：类属性的访问器与修改器，可以与变量命名风格对齐。Getter 使用变量名本身（不要加 `get_` 前缀），Setter 使用 `set_ + 变量名`。例如：
    ```cpp
    class MyClass {
     public:
      int count() const { return count_; }
      void set_count(int count) { count_ = count; }
     private:
      int count_;
    };
    ```
- **尽量让函数短小且职责单一**
  - 因为长函数在某些情况下是必要的，因此不对函数长度设硬性限制。
  - 然而，如果一个函数 **超过约 40 行**，必须评估是否可以在不破坏程序结构的前提下，拆分为更小、更易于调试和维护或更易复用的片段。
  - 拆分优先级建议：
    1. 提取“校验/解析/转换/核心逻辑/清理”等阶段为独立函数（命名必须能表达职责）。
    2. 把重复的错误处理、边界检查提取为 helper（避免在主流程中穿插大量 early-return）。
    3. 把多个并行的中间状态聚合到局部小结构体中，减少参数数量与跨行变量读写。

## 5. 注释风格（重要，严禁只写代码不写注释）

- **整体风格**
  - 统一使用 `//` 风格。
  - 除文件头部的版权声明外，正文代码统一采用简体中文注释。
  - 注释必须像正式文档一样编写，注意英文术语大小写和中文标点符号。
  - 完整的句子通常比句子片段或词组更具可读性。
- **注释写什么（避免“有注释但无信息”）**
  - **优先解释 why/how**：写清楚选择某种实现的原因、不变量、边界条件、错误处理策略、以及与 CPython/VM 约束的对齐点。
  - **避免复述 what**：不要把代码字面翻译成中文当注释（例如“i 自增”之类）。
- **文件注释**
  - 版权声明：所有新文件必须包含如下的标准许可证头。
  ```cpp
  // Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
  // Use of this source code is governed by a GNU-style license that can be
  // found in the LICENSE file.
  ```
  - 如果一个头文件（`.h`）声明了多个面向用户的抽象概念（如一组相关的类或函数），应在文件开头进行整体描述。
    - 注意：具体的类或函数文档应放在各自的声明处，不要堆砌在文件开头。
  - `.cc` 文件通常不需要文件级注释，除非它包含不属于任何头文件的独立实现细节。
- **类与结构体注释**
  - 必须写注释的情况：所有非显而易见（non-obvious）的类或结构体都必须有注释。
  - 内容：描述该类的用途（what it is for）以及如何使用（how it should be used）。
    - 可选：如果确认该类或结构体属于长时间内不会发生变动的基建，可以提供一小段示例代码。
  - 示例：
  ```cpp
  // 用于遍历 GargantuanTable 中的内容。
  // 示例:
  //    std::unique_ptr<GargantuanTableIterator> iter = table->NewIterator();
  //    for (iter->Seek("foo"); !iter->done(); iter->Next()) {
  //      process(iter->key(), iter->value());
  //    }
  class GargantuanTableIterator {
    ...
  };
  ```
- **声明处的函数注释**
  - 位置：几乎每个函数的声明前都应有注释。
  - 内容：描述函数做什么（what it does）以及如何使用（how to use it）。不要描述它是如何实现的（这是定义处注释的任务）。
  - 措辞：使用祈使句语气，以动词开头（隐含主语“该函数”）。例如：“打开文件”。
  - 必须包含的细节：
    - 输入参数和输出结果。尤其是当参数含义不明显时（例如传入 true/false 开关）。
    - 指针/Handle 参数是否允许为 `null`。
    - 如有：需特别说明的其他内容（例如使用该函数对性能的影响等）。
- **定义处的函数注释**
  - 内容：描述函数是如何工作的（how it does its job）。
  - 适用场景：解释复杂的实现逻辑、算法步骤、或者是为什么选择了这种实现方式而非其他方案。
  - 避免：不要简单重复声明处的注释。
- **函数实现细节注释**
  - 代码块：在晦涩、复杂或巧妙的代码段前添加解释性注释。
  - 行尾注释：如果是简短的说明，可以放在代码行尾，但要注意格式整洁。
- **TODO 注释**：使用全大写的 TODO，例如 `TODO: 当X模块完成后，移除此处逻辑`。

## 6. 与 AGENTS.md 的分工

- 本文档专注“通用代码风格与可维护性约定”。
- 与虚拟机实现强相关、且会影响正确性/GC/异常传播/依赖边界的硬规则，请以 [AGENTS.md](file:///e:/MyProject/S.A.A.U.S.O/AGENTS.md) 的架构与实现约束为准。
