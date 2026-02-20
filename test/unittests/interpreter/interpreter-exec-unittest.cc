// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/handles/handles.h"
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

TEST_F(BasicInterpreterTest, ExecPrint) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(exec('print(1)'))";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(1)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExecMutateGlobals) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
x = 1
exec("x = x + 41")
print(x)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(42)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExecWithGlobalsDict) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
d = {}
exec("a = 7", d)
print(d["a"])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(7)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExecWithGlobalsAndLocals) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
g = {"x": 10}
l = {}
exec("y = x + 1", g, l)
print(l["y"])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result, handle(PySmi::FromInt(11)));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExecGlobalsMustBeDictIsCatchable) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  exec("x = 1", 1)
except TypeError as e:
  print(str(e))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(
      expected_printv_result,
      PyString::NewInstance("exec() globals must be a dict, not int"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, ExecKeywordsMustBeStringsIsCatchable) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
try:
  exec("x = 1", **{1: 2})
except TypeError as e:
  print(str(e))
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::NewInstance();
  AppendExpected(expected_printv_result,
                 PyString::NewInstance("keywords must be strings"));
  ExpectPrintResult(expected_printv_result);
}

}  // namespace saauso::internal
