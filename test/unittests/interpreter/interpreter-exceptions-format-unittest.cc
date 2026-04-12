// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "src/objects/py-string.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, BaseExceptionStrAndReprUseArgsLayout) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
x = RuntimeError()
print(str(x))
print(repr(x))
y = RuntimeError(1, 2, 3)
print(str(y))
print(repr(y))
z = RuntimeError("WU-SUNFLOWER")
print(str(z))
print(repr(z))
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::New(isolate_);
  AppendExpected(expected, PyString::New(isolate_, ""));
  AppendExpected(expected, PyString::New(isolate_, "RuntimeError()"));
  AppendExpected(expected, PyString::New(isolate_, "(1, 2, 3)"));
  AppendExpected(expected, PyString::New(isolate_, "RuntimeError(1, 2, 3)"));
  AppendExpected(expected, PyString::New(isolate_, "WU-SUNFLOWER"));
  AppendExpected(expected,
                 PyString::New(isolate_, "RuntimeError('WU-SUNFLOWER')"));
  ExpectPrintResult(expected);
}

TEST_F(BasicInterpreterTest, PendingExceptionFormatUsesArgsLayout) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
raise RuntimeError(1, 2, 3)
)";

  RunScriptExpectExceptionContains(kSource, "RuntimeError: (1, 2, 3)",
                                   kTestFileName);
}

TEST_F(BasicInterpreterTest, TrueDivideByZeroRaisesZeroDivisionError) {
  HandleScope scope(isolate_);

  constexpr std::string_view kIntDivIntZero = R"(
10 / 0
)";
  RunScriptExpectExceptionContains(kIntDivIntZero,
                                   "ZeroDivisionError: division by zero",
                                   kTestFileName);

  constexpr std::string_view kIntDivFloatZero = R"(
10 / 0.0
)";
  RunScriptExpectExceptionContains(kIntDivFloatZero,
                                   "ZeroDivisionError: float division by zero",
                                   kTestFileName);

  constexpr std::string_view kFloatDivIntZero = R"(
10.0 / 0
)";
  RunScriptExpectExceptionContains(kFloatDivIntZero,
                                   "ZeroDivisionError: float division by zero",
                                   kTestFileName);

  constexpr std::string_view kFloatDivFloatZero = R"(
10.0 / 0.0
)";
  RunScriptExpectExceptionContains(kFloatDivFloatZero,
                                   "ZeroDivisionError: float division by zero",
                                   kTestFileName);
}

}  // namespace saauso::internal
