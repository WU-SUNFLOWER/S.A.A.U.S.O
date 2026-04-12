// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/heap/factory.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
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

}  // namespace saauso::internal
