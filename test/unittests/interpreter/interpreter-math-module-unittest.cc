// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <cmath>
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

TEST_F(BasicInterpreterTest, ImportBuiltinMath) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import math
print(math.sqrt(9))
print(math.floor(1.9))
print(math.ceil(1.1))
print(math.fabs(-3))
print(math.pow(2, 3))
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyFloat::New(isolate_, 3.0));
  AppendExpected(expected, handle(PySmi::FromInt(1)));
  AppendExpected(expected, handle(PySmi::FromInt(2)));
  AppendExpected(expected, PyFloat::New(isolate_, 3.0));
  AppendExpected(expected, PyFloat::New(isolate_, 8.0));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, MathConstants) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import math
print(math.pi)
print(math.e)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyFloat::New(isolate_, std::acos(-1.0)));
  AppendExpected(expected, PyFloat::New(isolate_, std::exp(1.0)));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, MathRejectKeywordArgs) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import math
math.sqrt(x=9)
)";

  RunScriptExpectExceptionContains(
      kSource, "math.sqrt() takes no keyword arguments", kTestFileName);
}

}  // namespace saauso::internal
