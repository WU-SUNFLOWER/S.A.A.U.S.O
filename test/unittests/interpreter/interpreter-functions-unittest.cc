// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string>
#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, IntToBinarySample) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def to_binary_string(value):
  result = ""
  
  if value == 0:
    return "0"
  
  while value > 0:
    result = str(value % 2) + result
    value //= 2
  return result

print(to_binary_string(2026))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "11111101010"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def add(a, b): 
    a += 233
    return a + b 

print(add(1, 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(236));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithDefaultArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b = 1, c = 2):
    return a + b + c

print(foo(10)) # 13
print(foo(100, 20, 30)) # 150
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(13));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(150));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithDefaultArgs2) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def make_func(x):
    def add(a, b = x): 
        return a + b 
    return add
add5 = make_func(5)
print(add5(10))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(15));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithDefaultArgsWithKwOverride) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b = 1, c = 2):
    return a + b + c

print(foo(10, c = 30))     # 41
print(foo(a = 10, c = 30)) # 41
print(foo(10, b = 20))     # 32
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(41));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(41));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(32));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionKeyArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    print(a)
    print(b)
    print(c)
foo(c = 10, b = 2, a = 6)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 6));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 2));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendPosArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def sum(*args):
    t = 0
    for i in args:
        t += i
    return t
print(sum(1, 2, 3, 4))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionMixedPosAndKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def bar(a, b, c):
    return a * 100 + b * 10 + c

print(bar(1, c = 3, b = 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 123));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(**kwargs):
    sum = 0
    for k, v in kwargs.items():
        if kwargs[k] is not v:
            print("fail")
        sum += v
    return sum
print(foo(a = 1, b = 4, c = 5))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendPosAndKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def calc(a, b, c = 89, *args, **kwargs):
    coeff = kwargs.get("coeff")
    if coeff is None:
        return 0
    t = a + b + c
    for i in args:
        t += i
    return coeff * t
print(calc(1, 2, 3, 4, coeff = 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 20));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendPosAndKwArgsWithBusinessScenario) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def send_email(sender, *recipients, **settings):
    print("Sender: " + sender)
    print("Recipients: " + ' '.join(recipients))

    # 提取配置，并提供默认值
    channel = settings.get('channel', 'email')
    priority = settings.get('priority', 'normal')

    print("Channel: " + channel + ", Priority: " + priority)

# 调用函数
send_email("Wang", "Mr.Song", "Mr.Liu", "Mr.Wu", channel="WeChat", priority="high")
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "Sender: Wang"));
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "Recipients: Mr.Song Mr.Liu Mr.Wu"));
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "Channel: WeChat, Priority: high"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendArgsEmptyPack) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def f(a, *args):
    return a + len(args)

def g(**kwargs):
    return len(kwargs)

def h(a, *args, **kwargs):
    return a + len(args) + len(kwargs)

print(f(10))
print(g())
print(h(1))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(10));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(0));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendArgsPackBehavior) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def f(a, *args):
    return a + len(args)

def g(a, **kwargs):
    return a + len(kwargs)

def h(a, *args, **kwargs):
    return a * 100 + len(args) * 10 + len(kwargs)

print(f(10, 1, 2))
print(g(7, x = 1, y = 2))
print(h(1, 2, 3, k = 4))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(12));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(9));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 121));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendKwArgsWithNamedParams) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b = 2, **kwargs):
    return a * 100 + b * 10 + len(kwargs)

print(foo(1, x = 7, y = 8))
print(foo(1, b = 3, x = 7))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 122));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 131));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MethodCallInjectSelf) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
a = "hello world"
print(a.upper())
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result,
                 PyString::New(isolate_, "HELLO WORLD"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallWithEmptyStarArgsAndStarKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c

print(foo(*(1, 2, 3), **{}))
print(foo(1, 2, 3, *(), **{}))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallWithStarArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c

t = (1, 2, 3)
print(foo(*t))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallWithStarKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c

d = {"a": 1, "b": 2, "c": 3}
print(foo(**d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(6));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallWithStarArgsAndStarKwArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a * 100 + b * 10 + c

t = (1,)
d = {"b": 2, "c": 3}
print(foo(*t, **d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 123));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallWithMultiStarKwArgsMerge) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a * 100 + b * 10 + c

d1 = {"b": 2}
d2 = {"c": 3}
print(foo(1, **d1, **d2))
print(foo(1, c = 3, **d1))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 123));
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 123));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FibRecursion) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def F(n):
    if n == 0:
        return 0
    elif n == 1:
        return 1
    else:
        return F(n - 1) + F(n - 2)

i = 0
while True:
  print(F(i))
  i += 1
  if i > 5:
    break
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(0));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(1));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(2));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(5));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MergeSortRecursion) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def merge(left, right):
    result = []
    i = 0
    j = 0
    while i < len(left):
        if j >= len(right):
            break
        if left[i] <= right[j]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    while i < len(left):
        result.append(left[i])
        i += 1
    while j < len(right):
        result.append(right[j])
        j += 1
    return result

def merge_sort(a):
    n = len(a)
    if n <= 1:
        return a
    mid = 0
    t = n
    while t > 1:
        t -= 2
        mid += 1
    left = []
    right = []
    i = 0
    while i < mid:
        left.append(a[i])
        i += 1
    while i < n:
        right.append(a[i])
        i += 1
    return merge(merge_sort(left), merge_sort(right))

data = [7, 3, 5, 2, 9, 1, 4, 8, 6, 0]
print(merge_sort(data))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  auto expected_sorted = PyList::New(isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(0),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(1),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(2),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(3),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(4),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(5),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(6),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(7),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(8),
                 isolate_);
  PyList::Append(expected_sorted, isolate_->factory()->NewSmiFromInt(9),
                 isolate_);
  AppendExpected(expected_printv_result, expected_sorted);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionCallErrors) {
  // 缺失必填位置参数
  std::string source = R"(
def foo(a, b = 1, c = 2):
    return a + b + c
foo()
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // **kwargs 导致 "got multiple values for argument"
  source = R"(
def foo(a, b, c):
    return a + b + c
d = {"a": 2}
foo(1, 2, 3, **d)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // DICT_MERGE 重复关键字
  source = R"(
def foo(a, b, c):
    return a + b + c
d1 = {"b": 2}
d2 = {"b": 3}
foo(1, **d1, **d2, c = 4)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // 关键字参数与 **kwargs 重复
  source = R"(
def foo(a, b, c):
    return a + b + c
d = {"b": 2}
foo(1, b = 3, c = 4, **d)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // 位置参数过多
  source = R"(
def bar(a, b, c):
    return a + b + c
bar(1, 2, 3, 4)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // 位置参数与关键字参数重复
  source = R"(
def bar(a, b, c):
    return a + b + c
bar(1, a = 2, b = 3, c = 4)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // *args 展开后与关键字参数重复
  source = R"(
def bar(a, b, c):
    return a + b + c
t = (1, 2, 3)
bar(*t, b = 2, c = 3)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);

  // 意外的关键字参数
  source = R"(
def bar(a, b, c):
    return a + b + c
bar(d = 1, a = 2, b = 3, c = 4)
)";
  RunScriptExpectExceptionContains(source, "TypeError", kTestFileName);
}

}  // namespace saauso::internal
