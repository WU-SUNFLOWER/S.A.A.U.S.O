// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
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

// ===== 排序基础语义 =====

TEST_F(BasicInterpreterTest, ListSortInt) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [3, 1, 2]
l.sort()
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto list = PyList::New(isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(1), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(2), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(3), isolate_);

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortReverse) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [3, 1, 2]
l.sort(reverse=True)
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto list = PyList::New(isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(3), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(2), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(1), isolate_);

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortKeyLenAndStability) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = ["aa", "b", "ccc", "dd"]
l.sort(key=len)
print(l)

s = ["bb", "aa"]
s.sort(key=len)
print(s)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto list = PyList::New(isolate_);
  PyList::Append(list, PyString::New(isolate_, "b"), isolate_);
  PyList::Append(list, PyString::New(isolate_, "aa"), isolate_);
  PyList::Append(list, PyString::New(isolate_, "dd"), isolate_);
  PyList::Append(list, PyString::New(isolate_, "ccc"), isolate_);

  auto stable_list = PyList::New(isolate_);
  PyList::Append(stable_list, PyString::New(isolate_, "bb"), isolate_);
  PyList::Append(stable_list, PyString::New(isolate_, "aa"), isolate_);

  AppendExpected(expected_printv_result, list);
  AppendExpected(expected_printv_result, stable_list);
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ListSortKey) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [108, 109, 105, 100, 106, 107]
l.sort(key=lambda x : x % 10)
print(l)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);

  auto list = PyList::New(isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(100), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(105), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(106), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(107), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(108), isolate_);
  PyList::Append(list, isolate_->factory()->NewSmiFromInt(109), isolate_);

  AppendExpected(expected_printv_result, list);
  ExpectPrintResult(expected_printv_result);
}

// ===== 排序错误语义 =====

TEST_F(BasicInterpreterTest, ListSortTypeErrorOnMixedTypes) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [1, "a"]
l.sort()
)";
  RunScriptExpectExceptionContains(kSource, "TypeError", kTestFileName);
}

}  // namespace saauso::internal
