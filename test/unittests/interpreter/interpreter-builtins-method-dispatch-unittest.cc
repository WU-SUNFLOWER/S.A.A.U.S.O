// Copyright 2026 the S.A.A.U.S.O project authors. All rights reserved.
// Use of this source code is governed by a GNU-style license that can be
// found in the LICENSE file.

#include <string_view>

#include "src/execution/isolate.h"
#include "src/handles/handles.h"
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

TEST_F(BasicInterpreterTest, CallMethodOfBuiltinType) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s = "hello"
t = s.upper()
print(s)
print(t)
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::New(isolate_, "hello"));
  AppendExpected(expected_printv_result, PyString::New(isolate_, "HELLO"));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallBuiltinMethodViaTypeObject) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
s = "hello"
print(str.upper(s))
l = [1, 2, 3]
list.append(l, 233)
print(l[3])
)";

  RunScript(kSource, kTestFileName);

  auto expected_printv_result = PyList::New(isolate_);
  AppendExpected(expected_printv_result, PyString::New(isolate_, "HELLO"));
  AppendExpected(expected_printv_result,
                 isolate_->factory()->NewSmiFromInt(233));
  ExpectPrintResult(expected_printv_result);
}

TEST_F(BasicInterpreterTest, CallBuiltinMethodViaTypeObjectWithoutReceiver) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
str.upper()
)";

  RunScriptExpectExceptionContains(
      kSource, "unbound method str.upper() needs an argument", kTestFileName);
}

TEST_F(BasicInterpreterTest, CallBuiltinMethodViaTypeObjectWithWrongReceiver) {
  HandleScope scope;

  constexpr std::string_view kSource = R"(
list.append(1, 2)
)";

  RunScriptExpectExceptionContains(
      kSource,
      "descriptor 'append' for 'list' objects doesn't apply to a 'int' object",
      kTestFileName);
}

}  // namespace saauso::internal
