// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/code/compiler.h"
#include "src/execution/isolate.h"
#include "src/handles/handles.h"
#include "src/interpreter/interpreter.h"
#include "src/objects/py-float.h"
#include "src/objects/py-list.h"
#include "src/objects/py-oddballs.h"
#include "src/objects/py-smi.h"
#include "src/objects/py-string.h"
#include "src/runtime/runtime-exceptions.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace saauso::internal {

TEST_F(BasicInterpreterTest, BuildClassErrors) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
try:
    __build_class__()
except TypeError as e:
    print(str(e))

def body():
    pass

try:
    __build_class__(body, 1)
except TypeError as e:
    print(str(e))
)";

  RunScript(kSource, kInterpreterTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(
      expected_printv_result,
      PyString::New(isolate_, "__build_class__: not enough arguments"));
  AppendExpected(
      expected_printv_result,
      PyString::New(isolate_, "__build_class__: name is not a string"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, BuildClassFuncMustBeFunction) {
  HandleScope scope(isolate_);

  constexpr std::string_view kSource = R"(
__build_class__(1, "A")
)";

  RunScriptExpectExceptionContains(kSource,
                                   "__build_class__: func must be a function",
                                   kInterpreterTestFileName);
}

}  // namespace saauso::internal
