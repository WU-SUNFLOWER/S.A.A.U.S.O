// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, FunctionWithArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def add(a, b): 
    a += 233
    return a + b 

print(add(1, 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(236)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithDefaultArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def foo(a, b = 1, c = 2):
    return a + b + c

print(foo(10)) # 13
print(foo(100, 20, 30)) # 150
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(13)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(150)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionWithDefaultArgs2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def make_func(x):
    def add(a, b = x): 
        return a + b 
    return add
add5 = make_func(5)
print(add5(10))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(15)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, FunctionKeyArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    print(a)
    print(b)
    print(c)
foo(c = 10, b = 2, a = 6)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyFloat::NewInstance(6));
  AppendExpected(expected_printv_result, PyFloat::NewInstance(2));
  AppendExpected(expected_printv_result, PyFloat::NewInstance(10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendPosArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def sum(*args):
    t = 0
    for i in args:
        t += i
    return t
print(sum(1, 2, 3, 4))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyFloat::NewInstance(10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendKwArgs) {
  HandleScope scope;

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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyFloat::NewInstance(10));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExtendPosAndKwArgs) {
  HandleScope scope;

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

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, PyFloat::NewInstance(20));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal

