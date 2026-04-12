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
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

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

}  // namespace saauso::internal
