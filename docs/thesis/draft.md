# 第1章 绪论

## 1.1 课题背景

## 1.2 国内外行业现状

## 1.3 研究的内容及意义


# 第2章 Python 语言的核心功能与 PVM 执行原理

## 2.1 PVM 执行原理综述

- Python 编译器前端首先将 Python 源代码解析成 AST 树，再通过后序遍历的方式，进一步翻译成二进制的、针对栈机的字节码。
- PVM 的运行过程类似于计算机的 CPU，会逐条读取字节码指令，并进行解释执行。
  - PVM 在执行过程中，会维护一个操作数栈。
  - 一条字节码在被解释执行时，PVM 首先会从操作数栈中弹出若干个元素，并按照对应字节码的规则进行运算或处理，再将所得结果（即这条字节码的执行结果）压回栈顶。


## 2.2 Python 中基本的控制流

PVM 中通过 `JUMP_IF_TRUE`、`JUMP_IF_FALSE`、 `JUMP_FORWARD` 等指令实现循环、条件等控制流。

## 2.3 Python 中的函数

### 2.3.1 简单函数的创建

不同于 C 语言和 C++，Python 中的函数是在运行时由 PVM 通过执行 `MAKE_FUNCTION` 字节码动态创建的。

### 2.3.2 栈帧与函数调用的原理

在 Python 中，每当执行函数调用时，PVM 都会创建一个新的栈帧（Stack Frame）。

栈帧中维护了当前函数的操作数栈、局部变量表、程序计数器等关键状态与数据。

当前函数执行结束时，当前栈帧的操作数栈上的栈顶元素即为函数的返回值。PVM 会将返回值取出并暂存并销毁当前栈帧，再将返回值压入调用方栈帧操作数栈，最后继续向下执行调用方函数。

### 2.3.3 扩展参数与键值对参数

### 2.3.4 函数闭包

### 2.3.5 常用的内建函数（Builtin Function）

- print
- len
- isinstance

## 2.4 Python 中的面向对象机制

### 2.4.1 综述

- Python 中既包括一系列内建类型，又允许用户自定义类
- 与多数面向对象编程语言类似，面向对象的三大特性（封装、继承、多态）在 Python 语言中均有体现。

### 2.4.1 基本内建类型

- object
- int
- float
- str
- list
- dict

### 2.4.3 类的封装

封装指将数据（属性）和操作数据的方法捆绑在一起，并隐藏内部实现细节，只对外暴露必要的接口。

与大多数面向对象的编程语言类似，Python 允许用户为自定义类添加方法与属性，从而实现数据驱动方法执行。

但不同于 C++、Java 等其他的主流编程语言，Python 语言出于动态性和灵活性的考虑，并不支持用户设置类的方法与属性的可见性（例如 public、protected、private 等）。

### 2.4.4 类的继承

与大多数面向对象的编程语言类似，Python 继承允许一个类（子类）基于另一个类（父类）来定义，从而复用代码并建立层次关系。

```python
class Animal:
    def __init__(self, name):
        self.name = name

    def speak(self):
        return "Some sound"

class Dog(Animal):                     # 单继承
    def speak(self):                   # 重写 speak
        return f"{self.name} says Woof!"

    def wag_tail(self):
        return "Tail wagging"

class Cat(Animal):
    def speak(self):
        return f"{self.name} says Meow!"

dog = Dog("Buddy")
print(dog.speak())      # Buddy says Woof!
print(dog.wag_tail())   # Tail wagging

cat = Cat("Kitty")
print(cat.speak())      # Kitty says Meow!
```

特别地，Python 是一门支持多继承的编程语言。一个子类可以继承多个父类，class C(A, B):。

为了确定再多个父类存在同名方法的情况下，子类应当选取哪个父类的方法进行继承，Python 中引入了 MRO 序列（Method Resolution Order）的概念。

当子类创建时，PVM 会执行 C3 线性化算法对该类包括父类在内的所有祖先类进行排序，所得结果即为 MRO 序列。

当子类尝试调用继承所得的方法时，PVM 会遍历 MRO 序列中的祖先类，并尝试在祖先类中查找目标方法。第一个找到的目标方法即为结果。

### 2.4.5 类的多态

多态指同一个接口（方法名）在不同对象上表现出不同的行为。Python 是动态类型语言，多态体现得更加自然，主要依赖鸭子类型和方法重写。
- 方法重写：子类重写父类方法后，调用相同方法名会执行子类版本。
- 鸭子类型：不关心对象的具体类型，只关心它是否具有所需的方法或属性。“如果它走起来像鸭子、叫起来像鸭子，那么它就是鸭子。” 这使得 Python 的多态不依赖于继承体系，只要对象实现了相应方法，就可以被统一处理。

```python
# 多态示例：不同对象调用相同接口
def make_sound(animal):      # 函数不限定参数类型
    print(animal.speak())    # 只要 animal 有 speak() 方法即可

make_sound(Dog("Rex"))       # Rex says Woof!
make_sound(Cat("Lucy"))      # Lucy says Meow!
make_sound(Animal("Unknown"))# Some sound

# 鸭子类型示例
class Robot:
    def speak(self):         # 没有继承 Animal，但实现了 speak
        return "Beep boop"

make_sound(Robot())          # Beep boop —— 也能工作

# 另一个例子：len() 函数的多态
print(len("hello"))          # 字符串：5
print(len([1, 2, 3]))        # 列表：3
print(len({"a": 1}))         # 字典：1
# 这些类型都实现了 __len__ 方法，所以 len() 可以统一处理
```

## 2.5 Python 中的异常机制

## 2.6 Python 中的模块机制

# 第3章 S.A.A.U.S.O VM 的设计与实现

# 第4章 S.A.A.U.S.O VM 的测试与结论

# 第5章 S.A.A.U.S.O VM 的能力边界与未来展望

# 参考文献