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

TEST_F(BasicInterpreterTest, SimpleClosure1) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def func():
    x = 3
    def say():
        print(x)
    return say
f = func()
f()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, SimpleClosure2) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
def func(x = 5):
    def say():
        print(x)
    x = 3
    return say
say = func()
say()
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(3)));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
