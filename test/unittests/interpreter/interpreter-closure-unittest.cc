// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
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

TEST_F(BasicInterpreterTest, SimpleClosure1) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def func():
    x = 3
    def say():
        print(x)
    return say
f = func()
f()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SimpleClosure2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def func(x = 5):
    def say():
        print(x)
    x = 3
    return say
say = func()
say()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, NestedClosures) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def foo(x, y):
    def bar(i):
        nonlocal x
        x += 1
        print(x)
        print(i)
        def baz(j):
            nonlocal y
            y += 1
            print(y)
            print(j)
        return baz
    return bar

bar_instance = foo(12, 25)
baz_instance = bar_instance(19)
baz_instance(14)
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(13)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(19)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(26)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(14)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SimpleDecorator) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def call_cnt(fn):
    cnt = 0
    def inner_func(*args):
        nonlocal cnt
        cnt += 1
        print(cnt)
        return fn(*args)

    return inner_func

@call_cnt
def add(a, b = 2): 
    return a + b 

print(add(1, 2))
print(add(2, 3))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(5)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, UnboundFreeVarFailsFast) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def outer():
    def inner():
        print(x)
    inner()
    x = 1
outer()
)";

  isolate()->interpreter()->Run(
      Compiler::CompileSource(isolate(), kSource, kInterpreterTestFileName));
  ASSERT_TRUE(isolate()->HasPendingException());
  auto f = Runtime_FormatPendingExceptionForStderr();
  std::string msg(f->buffer(), static_cast<size_t>(f->length()));
  EXPECT_NE(msg.find("free variable"), std::string::npos);
  isolate()->exception_state()->Clear();
}

TEST_F(BasicInterpreterTest, SharedCellAcrossClosures) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def outer():
    x = 0
    def inc():
        nonlocal x
        x += 1
        return x
    def get():
        return x
    return inc, get

inc, get = outer()
print(inc())
print(get())
print(inc())
print(get())
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(2)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MultiLevelNestedClosure) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def a():
    x = 7
    def b():
        def c():
            print(x)
        return c
    return b()

a()()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
