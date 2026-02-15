// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
#include "src/objects/py-list.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

namespace {

constexpr std::string_view kTestFileName = kInterpreterTestFileName;

}  // namespace

TEST_F(BasicInterpreterTest, ImportBuiltinTime) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
import time

t1 = time.perf_counter()
time.sleep(0.01)
t2 = time.perf_counter()
print(t2 > t1)
print(time.monotonic() >= 0)
print(time.time() > 0)
)";

  RunScript(kSource, kTestFileName);

  auto expected = PyList::NewInstance();
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, PyTrueObject());
  AppendExpected(expected, PyTrueObject());
  ExpectPrintResult(expected);
}

}  // namespace saauso::internal

