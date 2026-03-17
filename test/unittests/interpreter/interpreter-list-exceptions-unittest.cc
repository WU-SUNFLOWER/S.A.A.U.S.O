// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "test/unittests/test-helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

// ===== 异常传播语义 =====

TEST_F(BasicInterpreterTest, ListIndexPropagatesExceptionFromEq) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
class X:
  def __eq__(self, other):
    raise RuntimeError("boom")

l = [X()]
l.index(X())
)";

  RunScriptExpectExceptionContains(kSource, "RuntimeError", kTestFileName);
}

// ===== 错误信息语义 =====

TEST_F(BasicInterpreterTest, ListIndexDictNotFoundValueError) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
l = [1,2,3,4,5]
d = (2, 0, 2, 6)
l.index(d)
)";

  RunScriptExpectExceptionContains(
      kSource, "ValueError: (2, 0, 2, 6) is not in list", kTestFileName);
}

}  // namespace saauso::internal
