// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, FunctionCallErrorMissingRequiredPositionalArg) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b = 1, c = 2):
    return a + b + c
foo()
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorKwargsMultipleValues) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c
d = {"a": 2}
foo(1, 2, 3, **d)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorDictMergeDuplicateKeyword) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c
d1 = {"b": 2}
d2 = {"b": 3}
foo(1, **d1, **d2, c = 4)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorKeywordAndKwargsDuplicate) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def foo(a, b, c):
    return a + b + c
d = {"b": 2}
foo(1, b = 3, c = 4, **d)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorTooManyPositionalArgs) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def bar(a, b, c):
    return a + b + c
bar(1, 2, 3, 4)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorPositionalAndKeywordDuplicate) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def bar(a, b, c):
    return a + b + c
bar(1, a = 2, b = 3, c = 4)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest,
       FunctionCallErrorStarArgsExpandedThenKeywordDuplicate) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def bar(a, b, c):
    return a + b + c
t = (1, 2, 3)
bar(*t, b = 2, c = 3)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, FunctionCallErrorUnexpectedKeywordArg) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
def bar(a, b, c):
    return a + b + c
bar(d = 1, a = 2, b = 3, c = 4)
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

}  // namespace saauso::internal
