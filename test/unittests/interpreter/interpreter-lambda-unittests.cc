// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include "src/handles/handles.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

TEST_F(BasicInterpreterTest, LambdaImmediateCall) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
print((lambda x: x + 1)(2))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LambdaAssignedAndCalled) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
f = lambda a, b: a * 100 + b
print(f(1, 2))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 102));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LambdaAsArgument) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def apply(f, x):
    return f(x)
print(apply(lambda y: y * 2, 6))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyFloat::New(isolate_, 12));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, LambdaWithDefaultArgsCapture) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def make(x):
    return lambda a, b = x: a + b
f = make(5)
print(f(10))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(15)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
