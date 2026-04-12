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

    channel = settings.get('channel', 'email')
    priority = settings.get('priority', 'normal')

    print("Channel: " + channel + ", Priority: " + priority)

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

}  // namespace saauso::internal
