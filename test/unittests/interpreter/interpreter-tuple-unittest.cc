// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/py-list.h"
#include "src/objects/py-smi.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, TupleIndexPropagatesExceptionFromEq) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
class X:
  def __eq__(self, other):
    raise RuntimeError("boom")

t = (X(),)
t.index(X())
)";

  RunScriptExpectExceptionContains(kSource, "RuntimeError", kTestFileName);
}

TEST_F(BasicInterpreterTest, BuildTuple) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
d = (1,2,3,4)
print(d[2])
print(len(d))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(3));
  AppendExpected(expected_printv_result, isolate_->factory()->NewSmiFromInt(4));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
