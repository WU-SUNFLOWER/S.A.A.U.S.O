// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, CallUsesLoadGlobalNullPrefix) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def f():
    print(1)
f()
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, MethodCallUsesLoadAttrMethodShape) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def f():
    l = []
    l.append(1)
    print(len(l))
f()
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ManyMethodCallsDoNotAllocateBoundMethods) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def f():
    l = []
    i = 0
    while i < 100:
        l.append(i)
        i = i + 1
    print(len(l))
f()
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(100)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, DecoratorAndCallFunctionExStackLayout) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def deco(f):
    def wrap(*args, **kwargs):
        return f(*args, **kwargs)
    return wrap

@deco
def add(a, b):
    return a + b

print(add(1, 2))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
